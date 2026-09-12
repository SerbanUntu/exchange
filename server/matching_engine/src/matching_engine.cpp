#include "server/matching_engine/matching_engine.hpp"

#include "common/util/overloads.hpp"
#include "server/common/model/action/order_added_action.hpp"
#include "server/common/model/action/order_downsized_action.hpp"
#include "server/common/model/action/order_removed_action.hpp"
#include "server/common/model/action/order_upsized_action.hpp"
#include "server/common/model/action/trade_action.hpp"
#include "server/common/model/event/amend_limit_order_event.hpp"
#include "server/common/model/event/cancel_limit_order_event.hpp"
#include "server/common/model/event/new_limit_order_event.hpp"
#include "server/common/model/event/new_market_order_event.hpp"

#include <optional>
#include <unordered_map>
#include <vector>

namespace exchange::server
{
MatchingEngine::MatchingEngine(MPSCQueue<Event> &inQueue, SPMCQueue<Action> &outQueue,
                               const std::span<const SecurityId> securities)
    : inQueue(inQueue), outQueue(outQueue)
{
    for (const auto securityId : securities)
    {
        state.try_emplace(securityId);
    }
}

std::vector<Action> MatchingEngine::processEvent(const Event &event)
{
    auto &orderBook = state.at(event.header.securityId);
    std::vector<Action> result;

    std::visit(
        overloads{
            [&](const NewLimitOrderEvent &eventBody) {
                const auto [addedOrder, trades] =
                    orderBook.addOrder(event.header.orderId, eventBody.side, eventBody.price, eventBody.quantity,
                                       eventBody.tif, OrderType::LIMIT);
                for (const auto &[aggressorId, restingId, price, quantity, aggressorSide, tradeResult] : trades)
                {
                    result.emplace_back(Action::Header{.orderId = aggressorId,
                                                       .securityId = event.header.securityId,
                                                       .timestamp = std::chrono::system_clock::now()},
                                        TradeAction{.restingId = restingId,
                                                    .price = price,
                                                    .quantity = quantity,
                                                    .aggressorSide = aggressorSide});
                    if (tradeResult == Trade::TradeResult::RESTING_FILLED || tradeResult == Trade::TradeResult::BOTH)
                    {
                        result.emplace_back(Action::Header{.orderId = restingId,
                                                           .securityId = event.header.securityId,
                                                           .timestamp = std::chrono::system_clock::now()},
                                            OrderRemovedAction{});
                    }
                }
                if (addedOrder)
                {
                    result.emplace_back(Action::Header{.orderId = event.header.orderId,
                                                       .securityId = event.header.securityId,
                                                       .timestamp = std::chrono::system_clock::now()},
                                        OrderAddedAction{.price = eventBody.price,
                                                         .totalQuantity = eventBody.quantity,
                                                         .filledQuantity = addedOrder->filledQuantity,
                                                         .side = eventBody.side});
                }
            },
            [&](const NewMarketOrderEvent &eventBody) {
                const auto [addedOrder, trades] =
                    orderBook.addOrder(event.header.orderId, eventBody.side, std::nullopt, eventBody.quantity,
                                       TimeInForce::IOC, OrderType::MARKET);
                for (const auto &[aggressorId, restingId, price, quantity, aggressorSide, tradeResult] : trades)
                {
                    result.emplace_back(Action::Header{.orderId = aggressorId,
                                                       .securityId = event.header.securityId,
                                                       .timestamp = std::chrono::system_clock::now()},
                                        TradeAction{.restingId = restingId,
                                                    .price = price,
                                                    .quantity = quantity,
                                                    .aggressorSide = aggressorSide});
                    if (tradeResult == Trade::TradeResult::RESTING_FILLED || tradeResult == Trade::TradeResult::BOTH)
                    {
                        result.emplace_back(Action::Header{.orderId = restingId,
                                                           .securityId = event.header.securityId,
                                                           .timestamp = std::chrono::system_clock::now()},
                                            OrderRemovedAction{});
                    }
                }
            },
            [&](const AmendLimitOrderEvent &eventBody) {
                if (eventBody.isPriceAmended)
                {
                    const OrderBook::AmendOrderResult amendResult =
                        eventBody.isQuantityAmended
                            ? orderBook.amendOrder(event.header.orderId, eventBody.quantity, eventBody.price)
                            : orderBook.amendOrder(event.header.orderId, eventBody.price);
                    if (amendResult.status == OrderBook::AmendOrderStatus::CANNOT_AMEND)
                        return;
                    if (amendResult.status == OrderBook::AmendOrderStatus::AMENDED_IN_PLACE &&
                        eventBody.isQuantityAmended)
                    {
                        result.emplace_back(Action::Header{.orderId = event.header.orderId,
                                                           .securityId = event.header.securityId,
                                                           .timestamp = std::chrono::system_clock::now()},
                                            OrderDownsizedAction{.newQuantity = eventBody.quantity});
                        return;
                    }

                    result.emplace_back(Action::Header{.orderId = event.header.orderId,
                                                       .securityId = event.header.securityId,
                                                       .timestamp = std::chrono::system_clock::now()},
                                        OrderRemovedAction{});
                    if (amendResult.status == OrderBook::AmendOrderStatus::REMOVED)
                        return;

                    const auto &[addedOrder, trades] = amendResult.tradesAfterLosingPriority;
                    for (const auto &[aggressorId, restingId, price, quantity, aggressorSide, tradeResult] : trades)
                    {
                        result.emplace_back(Action::Header{.orderId = aggressorId,
                                                           .securityId = event.header.securityId,
                                                           .timestamp = std::chrono::system_clock::now()},
                                            TradeAction{.restingId = restingId,
                                                        .price = price,
                                                        .quantity = quantity,
                                                        .aggressorSide = aggressorSide});
                        if (tradeResult == Trade::TradeResult::RESTING_FILLED ||
                            tradeResult == Trade::TradeResult::BOTH)
                        {
                            result.emplace_back(Action::Header{.orderId = restingId,
                                                               .securityId = event.header.securityId,
                                                               .timestamp = std::chrono::system_clock::now()},
                                                OrderRemovedAction{});
                        }
                    }
                    if (addedOrder)
                    {
                        result.emplace_back(Action::Header{.orderId = event.header.orderId,
                                                           .securityId = event.header.securityId,
                                                           .timestamp = std::chrono::system_clock::now()},
                                            OrderAddedAction{.price = addedOrder->price,
                                                             .totalQuantity = addedOrder->totalQuantity,
                                                             .filledQuantity = addedOrder->filledQuantity,
                                                             .side = addedOrder->side});
                    }
                }
                else if (eventBody.isQuantityAmended)
                {
                    switch (orderBook.amendOrder(event.header.orderId, eventBody.quantity))
                    {
                    case OrderBook::AmendOrderStatus::AMENDED_IN_PLACE: {
                        result.emplace_back(Action::Header{.orderId = event.header.orderId,
                                                           .securityId = event.header.securityId,
                                                           .timestamp = std::chrono::system_clock::now()},
                                            OrderDownsizedAction{.newQuantity = eventBody.quantity});
                        break;
                    }
                    case OrderBook::AmendOrderStatus::AMENDED: {
                        result.emplace_back(Action::Header{.orderId = event.header.orderId,
                                                           .securityId = event.header.securityId,
                                                           .timestamp = std::chrono::system_clock::now()},
                                            OrderUpsizedAction{.newQuantity = eventBody.quantity});
                        break;
                    }
                    case OrderBook::AmendOrderStatus::REMOVED: {
                        result.emplace_back(Action::Header{.orderId = event.header.orderId,
                                                           .securityId = event.header.securityId,
                                                           .timestamp = std::chrono::system_clock::now()},
                                            OrderRemovedAction{});
                        break;
                    }
                    default: {
                        break;
                    }
                    }
                }
            },
            [&](const CancelLimitOrderEvent &) {
                if (orderBook.removeOrder(event.header.orderId))
                {
                    result.emplace_back(Action::Header{.orderId = event.header.orderId,
                                                       .securityId = event.header.securityId,
                                                       .timestamp = std::chrono::system_clock::now()},
                                        OrderRemovedAction{});
                }
            },
        },
        event.body);
    return result;
}

} // namespace exchange::server
