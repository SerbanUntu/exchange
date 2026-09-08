#include "server/matching_engine/process_event.hpp"

#include "server/common/model/action/order_added_action.hpp"
#include "server/common/model/action/order_downsized_action.hpp"
#include "server/common/model/action/order_removed_action.hpp"
#include "server/common/model/action/order_upsized_action.hpp"
#include "server/common/model/action/trade_action.hpp"
#include "server/common/model/event/amend_limit_order_event.hpp"
#include "server/common/model/event/cancel_limit_order_event.hpp"
#include "server/common/model/event/new_limit_order_event.hpp"
#include "server/common/model/event/new_market_order_event.hpp"

namespace exchange::server
{
std::vector<std::unique_ptr<Action>> processEvent(const Event &event, std::unordered_map<SecurityId, OrderBook> &state)
{
    auto &orderBook = state.at(event.orderId.securityId);
    std::vector<std::unique_ptr<Action>> result;

    switch (event.getType())
    {
    case EventType::NEW_LIMIT_ORDER: {
        const auto &e = dynamic_cast<const NewLimitOrderEvent &>(event);
        const auto [addedOrder, trades] =
            orderBook.addOrder(e.orderId, e.side, e.price, e.quantity, e.tif, OrderType::LIMIT);
        for (const auto &[aggressorId, restingId, price, quantity, aggressorSide, tradeResult] : trades)
        {
            result.emplace_back(std::make_unique<TradeAction>(aggressorId, restingId, price, quantity, aggressorSide));
            if (tradeResult == Trade::TradeResult::RESTING_FILLED || tradeResult == Trade::TradeResult::BOTH)
            {
                result.emplace_back(std::make_unique<OrderRemovedAction>(restingId));
            }
        }
        if (addedOrder)
        {
            result.emplace_back(
                std::make_unique<OrderAddedAction>(e.orderId, e.price, e.quantity, addedOrder->filledQuantity, e.side));
        }
        break;
    }
    case EventType::NEW_MARKET_ORDER: {
        const auto &e = dynamic_cast<const NewMarketOrderEvent &>(event);
        const auto [addedOrder, trades] =
            orderBook.addOrder(e.orderId, e.side, std::nullopt, e.quantity, TimeInForce::IOC, OrderType::MARKET);
        for (const auto &[aggressorId, restingId, price, quantity, aggressorSide, tradeResult] : trades)
        {
            result.emplace_back(std::make_unique<TradeAction>(aggressorId, restingId, price, quantity, aggressorSide));
            if (tradeResult == Trade::TradeResult::RESTING_FILLED || tradeResult == Trade::TradeResult::BOTH)
            {
                result.emplace_back(std::make_unique<OrderRemovedAction>(restingId));
            }
        }
        break;
    }
    case EventType::AMEND_LIMIT_ORDER: {
        const auto &e = dynamic_cast<const AmendLimitOrderEvent &>(event);
        if (e.price.has_value())
        {
            const OrderBook::AmendOrderResult amendResult = e.quantity.has_value()
                                                                ? orderBook.amendOrder(e.orderId, *e.quantity, *e.price)
                                                                : orderBook.amendOrder(e.orderId, *e.price);
            if (amendResult.status == OrderBook::AmendOrderStatus::CANNOT_AMEND)
                break;
            if (amendResult.status == OrderBook::AmendOrderStatus::AMENDED_IN_PLACE)
            {
                result.emplace_back(std::make_unique<OrderDownsizedAction>(e.orderId, *e.quantity));
                break;
            }

            result.emplace_back(std::make_unique<OrderRemovedAction>(e.orderId));
            if (amendResult.status == OrderBook::AmendOrderStatus::REMOVED)
                break;

            const auto &[addedOrder, trades] = amendResult.tradesAfterLosingPriority;
            for (const auto &[aggressorId, restingId, price, quantity, aggressorSide, tradeResult] : trades)
            {
                result.emplace_back(
                    std::make_unique<TradeAction>(aggressorId, restingId, price, quantity, aggressorSide));
                if (tradeResult == Trade::TradeResult::RESTING_FILLED || tradeResult == Trade::TradeResult::BOTH)
                {
                    result.emplace_back(std::make_unique<OrderRemovedAction>(restingId));
                }
            }
            if (addedOrder)
            {
                result.emplace_back(std::make_unique<OrderAddedAction>(e.orderId, addedOrder->price,
                                                                       addedOrder->totalQuantity,
                                                                       addedOrder->filledQuantity, addedOrder->side));
            }
        }
        else
        {
            switch (orderBook.amendOrder(e.orderId, *e.quantity))
            {
            case OrderBook::AmendOrderStatus::AMENDED_IN_PLACE: {
                result.emplace_back(std::make_unique<OrderDownsizedAction>(e.orderId, *e.quantity));
                break;
            }
            case OrderBook::AmendOrderStatus::AMENDED: {
                result.emplace_back(std::make_unique<OrderUpsizedAction>(e.orderId, *e.quantity));
                break;
            }
            case OrderBook::AmendOrderStatus::REMOVED: {
                result.emplace_back(std::make_unique<OrderRemovedAction>(e.orderId));
                break;
            }
            default: {
                break;
            }
            }
        }
        break;
    }
    case EventType::CANCEL_LIMIT_ORDER: {
        const auto &e = dynamic_cast<const CancelLimitOrderEvent &>(event);
        if (orderBook.removeOrder(e.orderId))
        {
            result.emplace_back(std::make_unique<OrderRemovedAction>(e.orderId));
        }
        break;
    }
    }
    return result;
}

} // namespace exchange::server
