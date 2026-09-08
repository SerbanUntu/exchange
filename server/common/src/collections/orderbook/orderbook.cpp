#include "server/common/collections/orderbook/orderbook.hpp"

namespace exchange::server
{
// NOLINTNEXTLINE(bugprone-exception-escape)
OrderBook::AddOrderResult OrderBook::addOrder(OrderId orderId, const Side side, const std::optional<Price> price,
                                              const Quantity quantity, const TimeInForce timeInForce,
                                              const OrderType orderType) noexcept
{
    OrderBookMap &matchingBook = side == Side::BUY ? sellBook : buyBook;
    Quantity remainingQuantity = quantity;
    std::vector<Trade> trades;
    if (timeInForce == TimeInForce::FOK)
    {
        Quantity availableQuantity{0};
        auto bookIt = matchingBook.begin();
        while (bookIt != matchingBook.end())
        {
            if ((side == Side::BUY && bookIt->first > *price) || (side == Side::SELL && bookIt->first < *price))
            {
                break;
            }
            for (const auto &order : bookIt->second)
            {
                availableQuantity += order.remainingQuantity();
                if (availableQuantity >= quantity)
                    break;
            }
            if (availableQuantity >= quantity)
                break;
            ++bookIt;
        }
        if (availableQuantity < quantity)
        {
            return {.addedOrder = std::nullopt, .trades = {}};
        }
    }
    auto bookIt = matchingBook.begin();
    while (bookIt != matchingBook.end() && remainingQuantity > Quantity{0})
    {
        if (orderType == OrderType::LIMIT &&
            ((side == Side::BUY && bookIt->first > *price) || (side == Side::SELL && bookIt->first < *price)))
        {
            break;
        }
        auto &[currentPrice, orders] = *bookIt;
        auto orderIt = orders.begin();
        while (orderIt != orders.end() && remainingQuantity > Quantity{0})
        {
            const auto restingQuantity = orderIt->remainingQuantity();
            if (restingQuantity > remainingQuantity)
            {
                trades.emplace_back(orderId, orderIt->id, currentPrice, remainingQuantity, side,
                                    Trade::TradeResult::AGGRESSOR_FILLED);
                orderIt->filledQuantity += remainingQuantity;
                remainingQuantity = Quantity{0};
                break;
            }
            trades.emplace_back(orderId, orderIt->id, currentPrice, restingQuantity, side,
                                restingQuantity == remainingQuantity ? Trade::TradeResult::BOTH
                                                                     : Trade::TradeResult::RESTING_FILLED);
            remainingQuantity -= restingQuantity;
            orderLookup.erase(orderIt->id);
            orderIt = orders.erase(orderIt);
        }
        if (orders.empty())
        {
            bookIt = matchingBook.erase(bookIt);
        }
        else
        {
            ++bookIt;
        }
    }
    if (timeInForce == TimeInForce::GTC && remainingQuantity > Quantity{0})
    {
        OrderBookMap &restingBook = side == Side::BUY ? buyBook : sellBook;
        auto &priceLevel = restingBook[*price];
        priceLevel.emplace_back(orderId, *price, quantity, quantity - remainingQuantity, side);
        orderLookup.insert_or_assign(orderId, OrderLookupValue{&priceLevel, std::prev(priceLevel.end())});
        return {.addedOrder = Order{.id = orderId,
                                    .price = *price,
                                    .totalQuantity = quantity,
                                    .filledQuantity = quantity - remainingQuantity,
                                    .side = side},
                .trades = trades};
    }
    return {.addedOrder = std::nullopt, .trades = trades};
}

bool OrderBook::removeOrder(OrderId orderId) noexcept
{
    const auto lookupIt = orderLookup.find(orderId);
    if (lookupIt == orderLookup.end())
        return false;

    const auto [listPtr, orderIt] = lookupIt->second;
    const auto price = orderIt->price;
    const auto side = orderIt->side;
    listPtr->erase(orderIt);
    orderLookup.erase(lookupIt);

    if (listPtr->empty())
    {
        OrderBookMap &restingBook = side == Side::BUY ? buyBook : sellBook;
        restingBook.erase(price);
    }
    return true;
}
// NOLINTNEXTLINE(bugprone-exception-escape)
OrderBook::AmendOrderStatus OrderBook::amendOrder(OrderId orderId, const Quantity newQuantity) noexcept
{
    const auto lookupIt = orderLookup.find(orderId);
    if (lookupIt == orderLookup.end())
        return AmendOrderStatus::CANNOT_AMEND;

    const auto [listPtr, orderIt] = lookupIt->second;
    if (newQuantity == orderIt->totalQuantity || newQuantity < orderIt->filledQuantity)
        return AmendOrderStatus::CANNOT_AMEND;
    if (newQuantity == orderIt->filledQuantity)
    {
        removeOrder(orderId);
        return AmendOrderStatus::REMOVED;
    }
    if (newQuantity < orderIt->totalQuantity)
    {
        orderIt->totalQuantity = newQuantity;
        return AmendOrderStatus::AMENDED_IN_PLACE;
    }
    auto order = *orderIt;
    listPtr->erase(orderIt);
    order.totalQuantity = newQuantity;
    auto newOrderIt = listPtr->emplace(listPtr->end(), order);
    orderLookup[orderId] = std::pair{listPtr, newOrderIt};
    return AmendOrderStatus::AMENDED;
}
OrderBook::AmendOrderResult OrderBook::amendOrder(OrderId orderId, const Price newPrice) noexcept
{
    const auto lookupIt = orderLookup.find(orderId);
    if (lookupIt == orderLookup.end())
        return {.status = AmendOrderStatus::CANNOT_AMEND, .tradesAfterLosingPriority = {}};

    const auto [listPtr, orderIt] = lookupIt->second;
    if (newPrice == orderIt->price)
        return {.status = AmendOrderStatus::CANNOT_AMEND, .tradesAfterLosingPriority = {}};

    const auto order = *orderIt;
    removeOrder(orderId);
    return {.status = AmendOrderStatus::AMENDED,
            .tradesAfterLosingPriority =
                addOrder(orderId, order.side, newPrice, order.remainingQuantity(), TimeInForce::GTC, OrderType::LIMIT)};
}
OrderBook::AmendOrderResult OrderBook::amendOrder(OrderId orderId, const Quantity newQuantity,
                                                  const Price newPrice) noexcept
{
    const auto lookupIt = orderLookup.find(orderId);
    if (lookupIt == orderLookup.end())
        return {.status = AmendOrderStatus::CANNOT_AMEND, .tradesAfterLosingPriority = {}};

    const auto [listPtr, orderIt] = lookupIt->second;
    if (newPrice == orderIt->price && newQuantity == orderIt->totalQuantity)
        return {.status = AmendOrderStatus::CANNOT_AMEND, .tradesAfterLosingPriority = {}};
    if (newQuantity < orderIt->filledQuantity)
        return {.status = AmendOrderStatus::CANNOT_AMEND, .tradesAfterLosingPriority = {}};
    if (newQuantity == orderIt->filledQuantity)
    {
        removeOrder(orderId);
        return {.status = AmendOrderStatus::REMOVED, .tradesAfterLosingPriority = {}};
    }
    if (newPrice == orderIt->price && newQuantity < orderIt->totalQuantity)
    {
        return {.status = amendOrder(orderId, newQuantity), .tradesAfterLosingPriority = {}};
    }

    const auto order = *orderIt;
    removeOrder(orderId);
    return {.status = AmendOrderStatus::AMENDED,
            .tradesAfterLosingPriority = addOrder(orderId, order.side, newPrice, newQuantity - order.filledQuantity,
                                                  TimeInForce::GTC, OrderType::LIMIT)};
}

} // namespace exchange::server