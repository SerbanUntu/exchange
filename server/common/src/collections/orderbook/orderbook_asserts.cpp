#include "server/common/collections/orderbook/orderbook_asserts.hpp"

#include <cassert>

namespace exchange::server
{

void assertOrderBookInvariants(const OrderBook &orderBook, const bool onlyOrderBookAsserts)
{
    const auto buyIt = orderBook.getBuyBook().begin();
    const auto sellIt = orderBook.getSellBook().begin();
    if (buyIt != orderBook.getBuyBook().end() && sellIt != orderBook.getSellBook().end())
    {
        assert(buyIt->first < sellIt->first);
    }

    assert(orderBook.orderCount() == orderBook.orderLookupSize());

    if (onlyOrderBookAsserts)
        return;

    for (const auto &[price, orders] : orderBook.getBuyBook())
    {
        for (const auto &order : orders)
        {
            assertRestingOrderInvariants(order, Side::BUY, price);
        }
    }

    for (const auto &[price, orders] : orderBook.getSellBook())
    {
        for (const auto &order : orders)
        {
            assertRestingOrderInvariants(order, Side::SELL, price);
        }
    }
}

void assertRestingOrderInvariants(const Order &order, Side bookSide, Price priceLevel)
{
    assert(order.filledQuantity < order.totalQuantity);
    assert(order.side == bookSide);
    assert(order.price == priceLevel);
}

void assertMatchingInvariants(const Order &aggressor, Quantity aggressorFilledQuantity,
                              std::vector<std::pair<Trade, Order>> tradeRestingPairs)
{
    for (size_t i = 0; i < tradeRestingPairs.size(); ++i)
    {
        const auto &[trade, _] = tradeRestingPairs[i];
        // NOLINTNEXTLINE(bugprone-branch-clone)
        if (aggressor.side == Side::BUY)
        {
            assert(trade.price <= aggressor.price);
            if (i > 0)
                assert(trade.price >= tradeRestingPairs[i - 1].first.price);
        }
        else
        {
            assert(trade.price >= aggressor.price);
            if (i > 0)
                assert(trade.price <= tradeRestingPairs[i - 1].first.price);
        }
    }

    Quantity totalTradeQuantity{};
    for (const auto &[trade, restingOrder] : tradeRestingPairs)
    {
        assert(trade.price == restingOrder.price);
        totalTradeQuantity += trade.quantity;
    }
    assert(totalTradeQuantity <= aggressor.remainingQuantity());
    assert(totalTradeQuantity == aggressorFilledQuantity - aggressor.filledQuantity);

    for (size_t i = 0; i + 1 < tradeRestingPairs.size(); ++i)
    {
        const auto &[trade, restingOrder] = tradeRestingPairs[i];
        assert(trade.quantity == restingOrder.remainingQuantity());
    }

    if (!tradeRestingPairs.empty())
    {
        const auto &[lastTrade, lastRestingOrder] = tradeRestingPairs[tradeRestingPairs.size() - 1];
        assert(lastTrade.quantity <= lastRestingOrder.remainingQuantity());
    }
}

} // namespace exchange::server