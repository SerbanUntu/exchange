#include "reference_matching_engine.hpp"

#include "common/util/overloads.hpp"
#include "server/common/collections/orderbook/trade.hpp"
#include "server/common/model/action/order_added_action.hpp"
#include "server/common/model/action/order_downsized_action.hpp"
#include "server/common/model/action/order_removed_action.hpp"
#include "server/common/model/action/order_upsized_action.hpp"
#include "server/common/model/action/trade_action.hpp"
#include "server/common/model/event/amend_limit_order_event.hpp"
#include "server/common/model/event/cancel_limit_order_event.hpp"
#include "server/common/model/event/new_limit_order_event.hpp"
#include "server/common/model/event/new_market_order_event.hpp"

#include <algorithm>
#include <chrono>
#include <utility>
#include <variant>

namespace exchange::server
{

std::optional<std::size_t> ReferenceOrderBook::indexOf(const OrderId orderId) const
{
    for (std::size_t i = 0; i < orders.size(); ++i)
    {
        if (orders[i].id == orderId)
            return i;
    }
    return std::nullopt;
}

std::optional<std::size_t> ReferenceOrderBook::bestMatch(const Side side, const std::optional<Price> limit) const
{
    std::optional<std::size_t> best;
    for (std::size_t i = 0; i < orders.size(); ++i)
    {
        const auto &order = orders[i];
        if (order.side == side)
            continue;
        if (limit && (side == Side::BUY ? order.price > *limit : order.price < *limit))
            continue;
        if (!best || (side == Side::BUY ? order.price < orders[*best].price : order.price > orders[*best].price))
            best = i;
    }
    return best;
}

Quantity ReferenceOrderBook::availableQuantity(const Side side, const std::optional<Price> limit) const
{
    Quantity available{0};
    for (const auto &order : orders)
    {
        if (order.side == side)
            continue;
        if (limit && (side == Side::BUY ? order.price > *limit : order.price < *limit))
            continue;
        available += order.remainingQuantity();
    }
    return available;
}

void ReferenceOrderBook::erase(const OrderId orderId)
{
    std::vector<Order> kept;
    for (const auto &order : orders)
    {
        if (order.id != orderId)
            kept.push_back(order);
    }
    orders = std::move(kept);
}

OrderBook::AddOrderResult ReferenceOrderBook::addOrder(const OrderId orderId, const Side side,
                                                       const std::optional<Price> price, const Quantity totalQuantity,
                                                       const Quantity filledQuantity, const TimeInForce timeInForce,
                                                       const OrderType orderType)
{
    const std::optional<Price> limit = orderType == OrderType::LIMIT ? price : std::nullopt;
    Quantity remaining = totalQuantity - filledQuantity;

    if (timeInForce == TimeInForce::FOK && availableQuantity(side, limit) < remaining)
        return {};

    std::vector<Trade> trades;
    while (remaining > Quantity{0})
    {
        const auto match = bestMatch(side, limit);
        if (!match)
            break;
        auto &resting = orders[*match];
        const Quantity traded = std::min(remaining, resting.remainingQuantity());
        remaining -= traded;
        resting.filledQuantity += traded;
        const Quantity restingLeft = resting.remainingQuantity();

        auto result = Trade::TradeResult::AGGRESSOR_FILLED;
        if (restingLeft == Quantity{0})
            result = remaining == Quantity{0} ? Trade::TradeResult::BOTH : Trade::TradeResult::RESTING_FILLED;

        trades.push_back(Trade{.aggressorId = orderId,
                               .restingId = resting.id,
                               .price = resting.price,
                               .quantity = traded,
                               .remainingRestingQuantity = restingLeft,
                               .aggressorSide = side,
                               .result = result});
        if (restingLeft == Quantity{0})
            erase(trades.back().restingId);
    }

    if (orderType == OrderType::LIMIT && timeInForce == TimeInForce::GTC && price && remaining > Quantity{0})
    {
        const Order added{.id = orderId,
                          .price = *price,
                          .totalQuantity = totalQuantity,
                          .filledQuantity = totalQuantity - remaining,
                          .side = side};
        orders.push_back(added);
        return {.addedOrder = added, .trades = trades};
    }
    return {.addedOrder = std::nullopt, .trades = trades};
}

bool ReferenceOrderBook::removeOrder(const OrderId orderId)
{
    if (!indexOf(orderId))
        return false;
    erase(orderId);
    return true;
}

OrderBook::AmendOrderStatus ReferenceOrderBook::amendOrder(const OrderId orderId, const Quantity newQuantity)
{
    const auto index = indexOf(orderId);
    if (!index)
        return OrderBook::AmendOrderStatus::CANNOT_AMEND;
    auto order = orders[*index];
    if (newQuantity == order.totalQuantity || newQuantity < order.filledQuantity)
        return OrderBook::AmendOrderStatus::CANNOT_AMEND;
    if (newQuantity == order.filledQuantity)
    {
        erase(orderId);
        return OrderBook::AmendOrderStatus::REMOVED;
    }
    if (newQuantity < order.totalQuantity)
    {
        orders[*index].totalQuantity = newQuantity;
        return OrderBook::AmendOrderStatus::AMENDED_IN_PLACE;
    }
    erase(orderId);
    order.totalQuantity = newQuantity;
    orders.push_back(order);
    return OrderBook::AmendOrderStatus::AMENDED;
}

OrderBook::AmendOrderResult ReferenceOrderBook::amendOrder(const OrderId orderId, const Price newPrice)
{
    const auto order = getOrder(orderId);
    if (!order || order->price == newPrice)
        return {.status = OrderBook::AmendOrderStatus::CANNOT_AMEND, .tradesAfterLosingPriority = {}};
    return amendOrder(orderId, order->totalQuantity, newPrice);
}

OrderBook::AmendOrderResult ReferenceOrderBook::amendOrder(const OrderId orderId, const Quantity newQuantity,
                                                           const Price newPrice)
{
    const auto order = getOrder(orderId);
    if (!order || newQuantity < order->filledQuantity ||
        (newPrice == order->price && newQuantity == order->totalQuantity))
        return {.status = OrderBook::AmendOrderStatus::CANNOT_AMEND, .tradesAfterLosingPriority = {}};
    if (newQuantity == order->filledQuantity)
    {
        erase(orderId);
        return {.status = OrderBook::AmendOrderStatus::REMOVED, .tradesAfterLosingPriority = {}};
    }
    if (newPrice == order->price && newQuantity < order->totalQuantity)
        return {.status = amendOrder(orderId, newQuantity), .tradesAfterLosingPriority = {}};

    erase(orderId);
    return {.status = OrderBook::AmendOrderStatus::AMENDED,
            .tradesAfterLosingPriority = addOrder(orderId, order->side, newPrice, newQuantity, order->filledQuantity,
                                                  TimeInForce::GTC, OrderType::LIMIT)};
}

std::optional<Order> ReferenceOrderBook::getOrder(const OrderId orderId) const
{
    const auto index = indexOf(orderId);
    if (!index)
        return std::nullopt;
    return orders[*index];
}

std::size_t ReferenceOrderBook::orderCount() const
{
    return orders.size();
}

std::vector<Action> referenceProcessEvent(const Event &event, std::unordered_map<SecurityId, ReferenceOrderBook> &state)
{
    auto &book = state.at(event.header.securityId);
    const OrderId orderId = event.header.orderId;
    std::vector<Action> actions;

    const auto emit = [&](const OrderId id, const auto &body) {
        actions.push_back(Action{Action::Header{.orderId = id,
                                                .securityId = event.header.securityId,
                                                .timestamp = std::chrono::system_clock::now()},
                                 body});
    };
    const auto emitAddResult = [&](const OrderBook::AddOrderResult &result) {
        for (const auto &trade : result.trades)
        {
            emit(trade.aggressorId, TradeAction{.restingId = trade.restingId,
                                                .price = trade.price,
                                                .quantity = trade.quantity,
                                                .remainingRestingQuantity = trade.remainingRestingQuantity,
                                                .aggressorSide = trade.aggressorSide});
            if (trade.remainingRestingQuantity == Quantity{0})
                emit(trade.restingId, OrderRemovedAction{});
        }
        if (const auto &added = result.addedOrder)
        {
            emit(added->id, OrderAddedAction{.price = added->price,
                                             .totalQuantity = added->totalQuantity,
                                             .filledQuantity = added->filledQuantity,
                                             .side = added->side});
        }
    };

    std::visit(overloads{
                   [&](const NewLimitOrderEvent &body) {
                       emitAddResult(book.addOrder(orderId, body.side, body.price, body.quantity, Quantity{0}, body.tif,
                                                   OrderType::LIMIT));
                   },
                   [&](const NewMarketOrderEvent &body) {
                       emitAddResult(book.addOrder(orderId, body.side, std::nullopt, body.quantity, Quantity{0},
                                                   TimeInForce::IOC, OrderType::MARKET));
                   },
                   [&](const CancelLimitOrderEvent &) {
                       if (book.removeOrder(orderId))
                           emit(orderId, OrderRemovedAction{});
                   },
                   [&](const AmendLimitOrderEvent &body) {
                       if (!body.isPriceAmended && !body.isQuantityAmended)
                           return;
                       if (!body.isPriceAmended)
                       {
                           switch (book.amendOrder(orderId, body.quantity))
                           {
                           case OrderBook::AmendOrderStatus::AMENDED_IN_PLACE:
                               emit(orderId, OrderDownsizedAction{.newQuantity = body.quantity});
                               break;
                           case OrderBook::AmendOrderStatus::AMENDED:
                               emit(orderId, OrderUpsizedAction{.newQuantity = body.quantity});
                               break;
                           case OrderBook::AmendOrderStatus::REMOVED:
                               emit(orderId, OrderRemovedAction{});
                               break;
                           case OrderBook::AmendOrderStatus::CANNOT_AMEND:
                               break;
                           }
                           return;
                       }

                       const auto result = body.isQuantityAmended ? book.amendOrder(orderId, body.quantity, body.price)
                                                                  : book.amendOrder(orderId, body.price);
                       switch (result.status)
                       {
                       case OrderBook::AmendOrderStatus::AMENDED_IN_PLACE:
                           emit(orderId, OrderDownsizedAction{.newQuantity = body.quantity});
                           break;
                       case OrderBook::AmendOrderStatus::AMENDED:
                           emit(orderId, OrderRemovedAction{});
                           emitAddResult(result.tradesAfterLosingPriority);
                           break;
                       case OrderBook::AmendOrderStatus::REMOVED:
                           emit(orderId, OrderRemovedAction{});
                           break;
                       case OrderBook::AmendOrderStatus::CANNOT_AMEND:
                           break;
                       }
                   },
               },
               event.body);
    return actions;
}

} // namespace exchange::server
