#include "server/common/collections/orderbook/orderbook.hpp"

#include <iterator>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

namespace exchange::server
{
// NOLINTNEXTLINE(bugprone-exception-escape)
OrderBook::AddOrderResult OrderBook::addOrder(OrderId orderId, const Side side, const std::optional<Price> price,
                                              const Quantity totalQuantity, const Quantity filledQuantity,
                                              const TimeInForce timeInForce, const OrderType orderType) noexcept
{
    OrderBookMap &matchingBook = side == Side::BUY ? sellBook : buyBook;
    const Quantity quantity = totalQuantity - filledQuantity;
    Quantity remainingQuantity = quantity;
    std::vector<Trade> trades;
    if (timeInForce == TimeInForce::FOK)
    {
        Quantity availableQuantity{0};
        auto bookIt = matchingBook.begin();
        while (bookIt != matchingBook.end())
        {
            if (price.has_value() &&
                ((side == Side::BUY && bookIt->first > *price) || (side == Side::SELL && bookIt->first < *price)))
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
        if (orderType == OrderType::LIMIT && price.has_value() &&
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
                trades.emplace_back(orderId, orderIt->id, currentPrice, remainingQuantity,
                                    restingQuantity - remainingQuantity, side, Trade::TradeResult::AGGRESSOR_FILLED);
                orderIt->filledQuantity += remainingQuantity;
                remainingQuantity = Quantity{0};
                break;
            }
            trades.emplace_back(orderId, orderIt->id, currentPrice, restingQuantity, Quantity{0}, side,
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
    if (timeInForce == TimeInForce::GTC && remainingQuantity > Quantity{0} && price.has_value())
    {
        const auto restingPrice = *price;
        OrderBookMap &restingBook = side == Side::BUY ? buyBook : sellBook;
        auto &priceLevel = restingBook[restingPrice];
        priceLevel.emplace_back(orderId, restingPrice, totalQuantity, totalQuantity - remainingQuantity, side);
        orderLookup.insert_or_assign(orderId, OrderLookupValue{&priceLevel, std::prev(priceLevel.end())});
        return {.addedOrder = Order{.id = orderId,
                                    .price = restingPrice,
                                    .totalQuantity = totalQuantity,
                                    .filledQuantity = totalQuantity - remainingQuantity,
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
            .tradesAfterLosingPriority = addOrder(orderId, order.side, newPrice, order.totalQuantity,
                                                  order.filledQuantity, TimeInForce::GTC, OrderType::LIMIT)};
}
OrderBook::AmendOrderResult OrderBook::amendOrder(const OrderId orderId, const Quantity newQuantity,
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
            .tradesAfterLosingPriority = addOrder(orderId, order.side, newPrice, newQuantity, order.filledQuantity,
                                                  TimeInForce::GTC, OrderType::LIMIT)};
}

const OrderBookMap &OrderBook::getBuyBook() const noexcept
{
    return buyBook;
}

const OrderBookMap &OrderBook::getSellBook() const noexcept
{
    return sellBook;
}

std::optional<Order> OrderBook::getOrder(const OrderId orderId) const noexcept
{
    const auto it = orderLookup.find(orderId);
    if (it == orderLookup.end())
    {
        return std::nullopt;
    }
    return *it->second.second;
}

std::size_t OrderBook::orderCount() const noexcept
{
    std::size_t result{};
    for (const auto &orders : buyBook | std::views::values)
    {
        result += orders.size();
    }
    for (const auto &orders : sellBook | std::views::values)
    {
        result += orders.size();
    }
    return result;
}

std::size_t OrderBook::orderLookupSize() const noexcept
{
    return orderLookup.size();
}

} // namespace exchange::server