#include "server/common/collections/orderbook/orderbook.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace exchange::server
{
namespace
{
OrderId makeOrderId(const uint8_t seed)
{
    boost::uuids::uuid uuid{};
    uuid.data[0] = seed;
    return OrderId{uuid};
}

OrderBook::AddOrderResult addGtc(OrderBook &book, const OrderId orderId, const Side side, const uint64_t price,
                                 const uint64_t quantity)
{
    return book.addOrder(orderId, side, Price{price}, Quantity{quantity}, Quantity{0}, TimeInForce::GTC,
                         OrderType::LIMIT);
}

std::vector<Price> pricesOf(const OrderBookMap &book)
{
    std::vector<Price> prices;
    prices.reserve(book.size());
    for (const auto &[price, orders] : book)
    {
        prices.push_back(price);
    }
    return prices;
}

std::vector<OrderId> idsOf(const std::list<Order> &orders)
{
    std::vector<OrderId> ids;
    ids.reserve(orders.size());
    for (const auto &order : orders)
    {
        ids.push_back(order.id);
    }
    return ids;
}
} // namespace

TEST(OrderBookTest, AddOrderMatchesACrossingBuyWithARestingSell)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    const auto aggressor = makeOrderId(2);
    addGtc(book, resting, Side::SELL, 100, 10);

    const auto [addedOrder, trades] = addGtc(book, aggressor, Side::BUY, 100, 10);

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].aggressorId, aggressor);
    EXPECT_EQ(trades[0].restingId, resting);
    EXPECT_EQ(trades[0].price, Price{100});
    EXPECT_EQ(trades[0].quantity, Quantity{10});
    EXPECT_EQ(trades[0].remainingRestingQuantity, Quantity{0});
    EXPECT_EQ(trades[0].aggressorSide, Side::BUY);
    EXPECT_EQ(trades[0].result, Trade::TradeResult::BOTH);

    EXPECT_FALSE(addedOrder.has_value());
    EXPECT_FALSE(book.removeOrder(resting));
    EXPECT_FALSE(book.removeOrder(aggressor));
}

TEST(OrderBookTest, AddOrderMatchesACrossingSellWithARestingBuy)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    const auto aggressor = makeOrderId(2);
    addGtc(book, resting, Side::BUY, 100, 10);

    const auto [addedOrder, trades] = addGtc(book, aggressor, Side::SELL, 100, 10);

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].restingId, resting);
    EXPECT_EQ(trades[0].aggressorSide, Side::SELL);
    EXPECT_EQ(trades[0].result, Trade::TradeResult::BOTH);
    EXPECT_FALSE(addedOrder.has_value());
}

TEST(OrderBookTest, AddOrderRestsAGtcLimitOrderThatDoesNotCross)
{
    OrderBook book;
    const auto aggressor = makeOrderId(2);
    addGtc(book, makeOrderId(1), Side::SELL, 105, 5);

    const auto [addedOrder, trades] = addGtc(book, aggressor, Side::BUY, 100, 5);

    EXPECT_TRUE(trades.empty());
    ASSERT_TRUE(addedOrder.has_value());
    EXPECT_EQ(addedOrder->id, aggressor);
    EXPECT_EQ(addedOrder->price, Price{100});
    EXPECT_EQ(addedOrder->totalQuantity, Quantity{5});
    EXPECT_EQ(addedOrder->filledQuantity, Quantity{0});
    EXPECT_TRUE(book.removeOrder(aggressor));
}

TEST(OrderBookTest, AddOrderFillsAtTheRestingOrderPrice)
{
    OrderBook book;
    addGtc(book, makeOrderId(1), Side::SELL, 100, 5);

    const auto [addedOrder, trades] = addGtc(book, makeOrderId(2), Side::BUY, 110, 5);

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].price, Price{100});
    EXPECT_FALSE(addedOrder.has_value());
}

TEST(OrderBookTest, AddOrderMatchesTheBestPriceLevelFirst)
{
    OrderBook book;
    const auto expensive = makeOrderId(1);
    const auto cheap = makeOrderId(2);
    const auto middle = makeOrderId(3);
    addGtc(book, expensive, Side::SELL, 102, 1);
    addGtc(book, cheap, Side::SELL, 100, 1);
    addGtc(book, middle, Side::SELL, 101, 1);

    const auto trades = addGtc(book, makeOrderId(4), Side::BUY, 102, 3).trades;

    ASSERT_EQ(trades.size(), 3U);
    EXPECT_EQ(trades[0].restingId, cheap);
    EXPECT_EQ(trades[1].restingId, middle);
    EXPECT_EQ(trades[2].restingId, expensive);
}

TEST(OrderBookTest, AddOrderMatchesTheHighestBidFirst)
{
    OrderBook book;
    const auto low = makeOrderId(1);
    const auto high = makeOrderId(2);
    addGtc(book, low, Side::BUY, 100, 1);
    addGtc(book, high, Side::BUY, 101, 1);

    const auto trades = addGtc(book, makeOrderId(3), Side::SELL, 100, 2).trades;

    ASSERT_EQ(trades.size(), 2U);
    EXPECT_EQ(trades[0].restingId, high);
    EXPECT_EQ(trades[1].restingId, low);
}

TEST(OrderBookTest, AddOrderRespectsTimePriorityWithinAPriceLevel)
{
    OrderBook book;
    const auto first = makeOrderId(1);
    const auto second = makeOrderId(2);
    addGtc(book, first, Side::SELL, 100, 2);
    addGtc(book, second, Side::SELL, 100, 2);

    const auto trades = addGtc(book, makeOrderId(3), Side::BUY, 100, 3).trades;

    ASSERT_EQ(trades.size(), 2U);
    EXPECT_EQ(trades[0].restingId, first);
    EXPECT_EQ(trades[0].quantity, Quantity{2});
    EXPECT_EQ(trades[0].remainingRestingQuantity, Quantity{0});
    EXPECT_EQ(trades[1].restingId, second);
    EXPECT_EQ(trades[1].quantity, Quantity{1});
    EXPECT_EQ(trades[1].remainingRestingQuantity, Quantity{1});
    EXPECT_EQ(trades[1].result, Trade::TradeResult::AGGRESSOR_FILLED);
}

TEST(OrderBookTest, AddOrderPartiallyFillsARestingOrderAndLeavesItOnTheBook)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::SELL, 100, 10);

    const auto firstTrades = addGtc(book, makeOrderId(2), Side::BUY, 100, 4).trades;
    ASSERT_EQ(firstTrades.size(), 1U);
    EXPECT_EQ(firstTrades[0].quantity, Quantity{4});
    EXPECT_EQ(firstTrades[0].remainingRestingQuantity, Quantity{6});
    EXPECT_EQ(firstTrades[0].result, Trade::TradeResult::AGGRESSOR_FILLED);

    const auto secondTrades = addGtc(book, makeOrderId(3), Side::BUY, 100, 6).trades;
    ASSERT_EQ(secondTrades.size(), 1U);
    EXPECT_EQ(secondTrades[0].quantity, Quantity{6});
    EXPECT_EQ(secondTrades[0].remainingRestingQuantity, Quantity{0});
    EXPECT_EQ(secondTrades[0].result, Trade::TradeResult::BOTH);
    EXPECT_FALSE(book.removeOrder(resting));
}

TEST(OrderBookTest, AddOrderRestsTheUnfilledRemainderOfAGtcOrder)
{
    OrderBook book;
    const auto aggressor = makeOrderId(2);
    addGtc(book, makeOrderId(1), Side::SELL, 100, 3);

    const auto [addedOrder, trades] = addGtc(book, aggressor, Side::BUY, 100, 10);

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].quantity, Quantity{3});
    ASSERT_TRUE(addedOrder.has_value());
    EXPECT_EQ(addedOrder->totalQuantity, Quantity{10});
    EXPECT_EQ(addedOrder->filledQuantity, Quantity{3});

    const auto laterTrades = addGtc(book, makeOrderId(3), Side::SELL, 100, 7).trades;
    ASSERT_EQ(laterTrades.size(), 1U);
    EXPECT_EQ(laterTrades[0].restingId, aggressor);
    EXPECT_EQ(laterTrades[0].quantity, Quantity{7});
}

TEST(OrderBookTest, AddOrderDoesNotRestTheRemainderOfAnIocOrder)
{
    OrderBook book;
    const auto aggressor = makeOrderId(2);
    addGtc(book, makeOrderId(1), Side::SELL, 100, 2);

    const auto [addedOrder, trades] =
        book.addOrder(aggressor, Side::BUY, Price{100}, Quantity{5}, Quantity{0}, TimeInForce::IOC, OrderType::LIMIT);

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].quantity, Quantity{2});
    EXPECT_FALSE(addedOrder.has_value());
    EXPECT_FALSE(book.removeOrder(aggressor));

    EXPECT_TRUE(addGtc(book, makeOrderId(3), Side::SELL, 100, 3).trades.empty());
}

TEST(OrderBookTest, AddOrderRejectsAFokOrderThatCannotBeFilledCompletely)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    const auto aggressor = makeOrderId(2);
    addGtc(book, resting, Side::SELL, 100, 5);

    const auto [addedOrder, trades] =
        book.addOrder(aggressor, Side::BUY, Price{100}, Quantity{6}, Quantity{0}, TimeInForce::FOK, OrderType::LIMIT);

    EXPECT_TRUE(trades.empty());
    EXPECT_FALSE(addedOrder.has_value());
    const auto laterTrades = addGtc(book, makeOrderId(3), Side::BUY, 100, 5).trades;
    ASSERT_EQ(laterTrades.size(), 1U);
    EXPECT_EQ(laterTrades[0].restingId, resting);
    EXPECT_EQ(laterTrades[0].quantity, Quantity{5});
}

TEST(OrderBookTest, AddOrderFillsAFokOrderAcrossSeveralPriceLevels)
{
    OrderBook book;
    addGtc(book, makeOrderId(1), Side::SELL, 100, 2);
    addGtc(book, makeOrderId(2), Side::SELL, 101, 2);

    const auto [addedOrder, trades] = book.addOrder(makeOrderId(3), Side::BUY, Price{101}, Quantity{4}, Quantity{0},
                                                    TimeInForce::FOK, OrderType::LIMIT);

    ASSERT_EQ(trades.size(), 2U);
    EXPECT_EQ(trades[0].price, Price{100});
    EXPECT_EQ(trades[1].price, Price{101});
    EXPECT_FALSE(addedOrder.has_value());
}

TEST(OrderBookTest, AddOrderFillsAFokSellOrderAcrossSeveralPriceLevels)
{
    OrderBook book;
    addGtc(book, makeOrderId(1), Side::BUY, 101, 2);
    addGtc(book, makeOrderId(2), Side::BUY, 100, 2);

    const auto [addedOrder, trades] = book.addOrder(makeOrderId(3), Side::SELL, Price{100}, Quantity{4}, Quantity{0},
                                                    TimeInForce::FOK, OrderType::LIMIT);

    ASSERT_EQ(trades.size(), 2U);
    EXPECT_EQ(trades[0].price, Price{101});
    EXPECT_EQ(trades[1].price, Price{100});
    EXPECT_FALSE(addedOrder.has_value());
}

TEST(OrderBookTest, AddOrderIgnoresLiquidityOutsideTheLimitPriceForAFokOrder)
{
    OrderBook book;
    addGtc(book, makeOrderId(1), Side::SELL, 110, 5);

    const auto [addedOrder, trades] = book.addOrder(makeOrderId(2), Side::BUY, Price{100}, Quantity{5}, Quantity{0},
                                                    TimeInForce::FOK, OrderType::LIMIT);

    EXPECT_TRUE(trades.empty());
    EXPECT_FALSE(addedOrder.has_value());
}

TEST(OrderBookTest, AddOrderMatchesAMarketOrderAtEveryPriceLevel)
{
    OrderBook book;
    addGtc(book, makeOrderId(1), Side::SELL, 100, 2);
    addGtc(book, makeOrderId(2), Side::SELL, 500, 2);

    const auto [addedOrder, trades] = book.addOrder(makeOrderId(3), Side::BUY, std::nullopt, Quantity{3}, Quantity{0},
                                                    TimeInForce::IOC, OrderType::MARKET);

    ASSERT_EQ(trades.size(), 2U);
    EXPECT_EQ(trades[0].price, Price{100});
    EXPECT_EQ(trades[1].price, Price{500});
    EXPECT_EQ(trades[1].quantity, Quantity{1});
    EXPECT_FALSE(addedOrder.has_value());
}

TEST(OrderBookTest, AddOrderLeavesAMarketOrderUnfilledWhenTheBookIsEmpty)
{
    OrderBook book;
    const auto aggressor = makeOrderId(1);

    const auto [addedOrder, trades] = book.addOrder(aggressor, Side::BUY, std::nullopt, Quantity{5}, Quantity{0},
                                                    TimeInForce::IOC, OrderType::MARKET);

    EXPECT_TRUE(trades.empty());
    EXPECT_FALSE(addedOrder.has_value());
    EXPECT_FALSE(book.removeOrder(aggressor));
}

TEST(OrderBookTest, AddOrderDoesNotMatchTwoOrdersOnTheSameSide)
{
    OrderBook book;
    addGtc(book, makeOrderId(1), Side::BUY, 100, 5);

    const auto [addedOrder, trades] = addGtc(book, makeOrderId(2), Side::BUY, 100, 5);

    EXPECT_TRUE(trades.empty());
    EXPECT_TRUE(addedOrder.has_value());
}

TEST(OrderBookTest, RemoveOrderTakesARestingOrderOffTheBook)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::SELL, 100, 5);

    EXPECT_TRUE(book.removeOrder(resting));
    EXPECT_FALSE(book.removeOrder(resting));
    EXPECT_TRUE(addGtc(book, makeOrderId(2), Side::BUY, 100, 5).trades.empty());
}

TEST(OrderBookTest, RemoveOrderKeepsTheOtherOrdersAtThePriceLevel)
{
    OrderBook book;
    const auto first = makeOrderId(1);
    const auto second = makeOrderId(2);
    addGtc(book, first, Side::SELL, 100, 5);
    addGtc(book, second, Side::SELL, 100, 5);

    EXPECT_TRUE(book.removeOrder(first));

    const auto trades = addGtc(book, makeOrderId(3), Side::BUY, 100, 5).trades;
    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].restingId, second);
}

TEST(OrderBookTest, RemoveOrderReturnsFalseForAnUnknownOrder)
{
    OrderBook book;
    EXPECT_FALSE(book.removeOrder(makeOrderId(1)));
}

TEST(OrderBookTest, AmendOrderKeepsTimePriorityWhenTheQuantityDecreases)
{
    OrderBook book;
    const auto first = makeOrderId(1);
    const auto second = makeOrderId(2);
    addGtc(book, first, Side::SELL, 100, 10);
    addGtc(book, second, Side::SELL, 100, 10);

    EXPECT_EQ(book.amendOrder(first, Quantity{4}), OrderBook::AmendOrderStatus::AMENDED_IN_PLACE);

    const auto trades = addGtc(book, makeOrderId(3), Side::BUY, 100, 4).trades;
    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].restingId, first);
    EXPECT_EQ(trades[0].quantity, Quantity{4});
    EXPECT_EQ(trades[0].result, Trade::TradeResult::BOTH);
}

TEST(OrderBookTest, AmendOrderLosesTimePriorityWhenTheQuantityIncreases)
{
    OrderBook book;
    const auto first = makeOrderId(1);
    const auto second = makeOrderId(2);
    addGtc(book, first, Side::SELL, 100, 2);
    addGtc(book, second, Side::SELL, 100, 2);

    EXPECT_EQ(book.amendOrder(first, Quantity{5}), OrderBook::AmendOrderStatus::AMENDED);

    const auto trades = addGtc(book, makeOrderId(3), Side::BUY, 100, 7).trades;
    ASSERT_EQ(trades.size(), 2U);
    EXPECT_EQ(trades[0].restingId, second);
    EXPECT_EQ(trades[1].restingId, first);
    EXPECT_EQ(trades[1].quantity, Quantity{5});
}

TEST(OrderBookTest, AmendOrderKeepsTheFilledQuantityWhenTheQuantityIncreases)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::SELL, 100, 10);
    addGtc(book, makeOrderId(2), Side::BUY, 100, 4);

    EXPECT_EQ(book.amendOrder(resting, Quantity{12}), OrderBook::AmendOrderStatus::AMENDED);

    const auto trades = addGtc(book, makeOrderId(3), Side::BUY, 100, 8).trades;
    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].quantity, Quantity{8});
    EXPECT_EQ(trades[0].result, Trade::TradeResult::BOTH);
}

TEST(OrderBookTest, AmendOrderRemovesTheOrderWhenTheNewQuantityEqualsTheFilledQuantity)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::SELL, 100, 10);
    addGtc(book, makeOrderId(2), Side::BUY, 100, 4);

    EXPECT_EQ(book.amendOrder(resting, Quantity{4}), OrderBook::AmendOrderStatus::REMOVED);
    EXPECT_FALSE(book.removeOrder(resting));
    EXPECT_TRUE(addGtc(book, makeOrderId(3), Side::BUY, 100, 6).trades.empty());
}

TEST(OrderBookTest, AmendOrderRejectsAQuantityBelowTheFilledQuantity)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::SELL, 100, 10);
    addGtc(book, makeOrderId(2), Side::BUY, 100, 4);

    EXPECT_EQ(book.amendOrder(resting, Quantity{3}), OrderBook::AmendOrderStatus::CANNOT_AMEND);
    const auto trades = addGtc(book, makeOrderId(3), Side::BUY, 100, 6).trades;
    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].quantity, Quantity{6});
}

TEST(OrderBookTest, AmendOrderRejectsAnUnchangedQuantity)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::SELL, 100, 10);

    EXPECT_EQ(book.amendOrder(resting, Quantity{10}), OrderBook::AmendOrderStatus::CANNOT_AMEND);
    EXPECT_TRUE(book.removeOrder(resting));
}

TEST(OrderBookTest, AmendOrderRejectsAnUnknownOrder)
{
    OrderBook book;
    const auto unknown = makeOrderId(1);

    EXPECT_EQ(book.amendOrder(unknown, Quantity{5}), OrderBook::AmendOrderStatus::CANNOT_AMEND);
    EXPECT_EQ(book.amendOrder(unknown, Price{100}).status, OrderBook::AmendOrderStatus::CANNOT_AMEND);
    EXPECT_EQ(book.amendOrder(unknown, Quantity{5}, Price{100}).status, OrderBook::AmendOrderStatus::CANNOT_AMEND);
}

TEST(OrderBookTest, AmendOrderMovesTheOrderToTheNewPrice)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::BUY, 100, 5);

    const auto [status, addResult] = book.amendOrder(resting, Price{99});

    EXPECT_EQ(status, OrderBook::AmendOrderStatus::AMENDED);
    EXPECT_TRUE(addResult.trades.empty());
    ASSERT_TRUE(addResult.addedOrder.has_value());
    EXPECT_EQ(addResult.addedOrder->price, Price{99});

    EXPECT_TRUE(addGtc(book, makeOrderId(2), Side::SELL, 100, 5).trades.empty());
    const auto trades = addGtc(book, makeOrderId(3), Side::SELL, 99, 5).trades;
    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].restingId, resting);
}

TEST(OrderBookTest, AmendOrderMatchesImmediatelyWhenTheNewPriceCrosses)
{
    OrderBook book;
    const auto counterparty = makeOrderId(1);
    const auto resting = makeOrderId(2);
    addGtc(book, counterparty, Side::SELL, 105, 5);
    addGtc(book, resting, Side::BUY, 100, 5);

    const auto [status, addResult] = book.amendOrder(resting, Price{105});

    EXPECT_EQ(status, OrderBook::AmendOrderStatus::AMENDED);
    ASSERT_EQ(addResult.trades.size(), 1U);
    EXPECT_EQ(addResult.trades[0].aggressorId, resting);
    EXPECT_EQ(addResult.trades[0].restingId, counterparty);
    EXPECT_EQ(addResult.trades[0].price, Price{105});
    EXPECT_FALSE(addResult.addedOrder.has_value());
    EXPECT_FALSE(book.removeOrder(resting));
}

TEST(OrderBookTest, AmendOrderRejectsAnUnchangedPrice)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::BUY, 100, 5);

    EXPECT_EQ(book.amendOrder(resting, Price{100}).status, OrderBook::AmendOrderStatus::CANNOT_AMEND);
    EXPECT_TRUE(book.removeOrder(resting));
}

TEST(OrderBookTest, AmendOrderCarriesTheUnfilledQuantityToTheNewPrice)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::SELL, 100, 10);
    addGtc(book, makeOrderId(2), Side::BUY, 100, 4);

    EXPECT_EQ(book.amendOrder(resting, Price{99}).status, OrderBook::AmendOrderStatus::AMENDED);

    const auto trades = addGtc(book, makeOrderId(3), Side::BUY, 99, 10).trades;
    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].quantity, Quantity{6});
    EXPECT_EQ(trades[0].result, Trade::TradeResult::RESTING_FILLED);
}

TEST(OrderBookTest, AmendOrderAppliesTheNewQuantityAndTheNewPriceTogether)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::SELL, 100, 5);

    const auto [status, addResult] = book.amendOrder(resting, Quantity{8}, Price{99});

    EXPECT_EQ(status, OrderBook::AmendOrderStatus::AMENDED);
    ASSERT_TRUE(addResult.addedOrder.has_value());
    EXPECT_EQ(addResult.addedOrder->price, Price{99});
    EXPECT_EQ(addResult.addedOrder->totalQuantity, Quantity{8});

    const auto trades = addGtc(book, makeOrderId(2), Side::BUY, 99, 8).trades;
    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].quantity, Quantity{8});
}

TEST(OrderBookTest, AmendOrderKeepsTimePriorityWhenOnlyTheQuantityDecreasesAtTheSamePrice)
{
    OrderBook book;
    const auto first = makeOrderId(1);
    const auto second = makeOrderId(2);
    addGtc(book, first, Side::SELL, 100, 10);
    addGtc(book, second, Side::SELL, 100, 10);

    EXPECT_EQ(book.amendOrder(first, Quantity{4}, Price{100}).status, OrderBook::AmendOrderStatus::AMENDED_IN_PLACE);

    const auto trades = addGtc(book, makeOrderId(3), Side::BUY, 100, 4).trades;
    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].restingId, first);
}

TEST(OrderBookTest, AmendOrderRemovesTheOrderWhenTheNewQuantityEqualsTheFilledQuantityAtANewPrice)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::SELL, 100, 10);
    addGtc(book, makeOrderId(2), Side::BUY, 100, 4);

    const auto [status, addResult] = book.amendOrder(resting, Quantity{4}, Price{99});

    EXPECT_EQ(status, OrderBook::AmendOrderStatus::REMOVED);
    EXPECT_TRUE(addResult.trades.empty());
    EXPECT_FALSE(addResult.addedOrder.has_value());
    EXPECT_FALSE(book.removeOrder(resting));
}

TEST(OrderBookTest, AmendOrderRejectsAQuantityBelowTheFilledQuantityAtANewPrice)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::SELL, 100, 10);
    addGtc(book, makeOrderId(2), Side::BUY, 100, 4);

    const auto [status, addResult] = book.amendOrder(resting, Quantity{3}, Price{101});

    EXPECT_EQ(status, OrderBook::AmendOrderStatus::CANNOT_AMEND);
    EXPECT_TRUE(addResult.trades.empty());
    const auto order = book.getOrder(resting);
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->price, Price{100});
    EXPECT_EQ(order->totalQuantity, Quantity{10});
    EXPECT_EQ(order->filledQuantity, Quantity{4});
}

TEST(OrderBookTest, AddOrderOnlyMatchesTheUnfilledQuantityOfAPartiallyFilledOrder)
{
    OrderBook book;
    addGtc(book, makeOrderId(1), Side::SELL, 100, 10);

    const auto [addedOrder, trades] = book.addOrder(makeOrderId(2), Side::BUY, Price{100}, Quantity{10}, Quantity{7},
                                                    TimeInForce::GTC, OrderType::LIMIT);

    ASSERT_EQ(trades.size(), 1U);
    EXPECT_EQ(trades[0].quantity, Quantity{3});
    EXPECT_EQ(trades[0].remainingRestingQuantity, Quantity{7});
    EXPECT_FALSE(addedOrder.has_value());
}

TEST(OrderBookTest, AddOrderRestsAPartiallyFilledOrderWithItsOriginalQuantities)
{
    OrderBook book;
    const auto orderId = makeOrderId(1);

    const auto [addedOrder, trades] =
        book.addOrder(orderId, Side::BUY, Price{100}, Quantity{10}, Quantity{4}, TimeInForce::GTC, OrderType::LIMIT);

    EXPECT_TRUE(trades.empty());
    ASSERT_TRUE(addedOrder.has_value());
    EXPECT_EQ(addedOrder->totalQuantity, Quantity{10});
    EXPECT_EQ(addedOrder->filledQuantity, Quantity{4});
    const auto &resting = book.getBuyBook().at(Price{100}).front();
    EXPECT_EQ(resting.totalQuantity, Quantity{10});
    EXPECT_EQ(resting.filledQuantity, Quantity{4});
}

TEST(OrderBookTest, AddOrderDoesNotRestAnOrderThatIsAlreadyFullyFilled)
{
    OrderBook book;

    const auto [addedOrder, trades] = book.addOrder(makeOrderId(1), Side::BUY, Price{100}, Quantity{10}, Quantity{10},
                                                    TimeInForce::GTC, OrderType::LIMIT);

    EXPECT_TRUE(trades.empty());
    EXPECT_FALSE(addedOrder.has_value());
    EXPECT_EQ(book.orderCount(), 0U);
}

TEST(OrderBookTest, AmendOrderPreservesTheTotalAndFilledQuantitiesWhenRepricing)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::SELL, 100, 10);
    addGtc(book, makeOrderId(2), Side::BUY, 100, 4);

    const auto [status, addResult] = book.amendOrder(resting, Price{99});

    EXPECT_EQ(status, OrderBook::AmendOrderStatus::AMENDED);
    ASSERT_TRUE(addResult.addedOrder.has_value());
    EXPECT_EQ(addResult.addedOrder->totalQuantity, Quantity{10});
    EXPECT_EQ(addResult.addedOrder->filledQuantity, Quantity{4});
    EXPECT_EQ(book.amendOrder(resting, Quantity{8}), OrderBook::AmendOrderStatus::AMENDED_IN_PLACE);
    const auto &order = book.getSellBook().at(Price{99}).front();
    EXPECT_EQ(order.totalQuantity, Quantity{8});
    EXPECT_EQ(order.filledQuantity, Quantity{4});
}

TEST(OrderBookTest, AmendOrderAccumulatesTheFilledQuantityWhenARepricedOrderCrosses)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::BUY, 100, 10);
    addGtc(book, makeOrderId(2), Side::SELL, 100, 4);
    addGtc(book, makeOrderId(3), Side::SELL, 105, 3);

    const auto [status, addResult] = book.amendOrder(resting, Price{105});

    EXPECT_EQ(status, OrderBook::AmendOrderStatus::AMENDED);
    ASSERT_EQ(addResult.trades.size(), 1U);
    EXPECT_EQ(addResult.trades[0].quantity, Quantity{3});
    ASSERT_TRUE(addResult.addedOrder.has_value());
    EXPECT_EQ(addResult.addedOrder->totalQuantity, Quantity{10});
    EXPECT_EQ(addResult.addedOrder->filledQuantity, Quantity{7});
}

TEST(OrderBookTest, AmendOrderPreservesTheFilledQuantityWhenRepricingWithANewQuantity)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::SELL, 100, 10);
    addGtc(book, makeOrderId(2), Side::BUY, 100, 4);

    const auto [status, addResult] = book.amendOrder(resting, Quantity{12}, Price{99});

    EXPECT_EQ(status, OrderBook::AmendOrderStatus::AMENDED);
    ASSERT_TRUE(addResult.addedOrder.has_value());
    EXPECT_EQ(addResult.addedOrder->totalQuantity, Quantity{12});
    EXPECT_EQ(addResult.addedOrder->filledQuantity, Quantity{4});
    EXPECT_EQ(book.amendOrder(resting, Quantity{4}), OrderBook::AmendOrderStatus::REMOVED);
}

TEST(OrderBookTest, GetBuyBookListsBidsFromTheHighestPriceInTimePriority)
{
    OrderBook book;
    const auto first = makeOrderId(1);
    const auto second = makeOrderId(2);
    addGtc(book, makeOrderId(3), Side::BUY, 100, 1);
    addGtc(book, first, Side::BUY, 101, 1);
    addGtc(book, makeOrderId(4), Side::BUY, 102, 1);
    addGtc(book, second, Side::BUY, 101, 1);

    EXPECT_EQ(pricesOf(book.getBuyBook()), (std::vector{Price{102}, Price{101}, Price{100}}));
    EXPECT_EQ(idsOf(book.getBuyBook().at(Price{101})), (std::vector{first, second}));
    EXPECT_TRUE(book.getSellBook().empty());
}

TEST(OrderBookTest, GetSellBookListsAsksFromTheLowestPriceInTimePriority)
{
    OrderBook book;
    const auto first = makeOrderId(1);
    const auto second = makeOrderId(2);
    addGtc(book, makeOrderId(3), Side::SELL, 102, 1);
    addGtc(book, first, Side::SELL, 101, 1);
    addGtc(book, makeOrderId(4), Side::SELL, 100, 1);
    addGtc(book, second, Side::SELL, 101, 1);

    EXPECT_EQ(pricesOf(book.getSellBook()), (std::vector{Price{100}, Price{101}, Price{102}}));
    EXPECT_EQ(idsOf(book.getSellBook().at(Price{101})), (std::vector{first, second}));
    EXPECT_TRUE(book.getBuyBook().empty());
}

TEST(OrderBookTest, GetSellBookReflectsPartialFills)
{
    OrderBook book;
    addGtc(book, makeOrderId(1), Side::SELL, 100, 10);
    addGtc(book, makeOrderId(2), Side::BUY, 100, 4);

    const auto &order = book.getSellBook().at(Price{100}).front();
    EXPECT_EQ(order.totalQuantity, Quantity{10});
    EXPECT_EQ(order.filledQuantity, Quantity{4});
    EXPECT_TRUE(book.getBuyBook().empty());
}

TEST(OrderBookTest, GetBuyBookDropsEmptyPriceLevels)
{
    OrderBook book;
    const auto cancelled = makeOrderId(1);
    addGtc(book, cancelled, Side::BUY, 100, 5);
    addGtc(book, makeOrderId(2), Side::BUY, 101, 5);
    addGtc(book, makeOrderId(3), Side::BUY, 102, 5);

    book.removeOrder(cancelled);
    addGtc(book, makeOrderId(4), Side::SELL, 102, 5);

    EXPECT_EQ(pricesOf(book.getBuyBook()), (std::vector{Price{101}}));
}

TEST(OrderBookTest, OrderCountIsZeroForAnEmptyBook)
{
    const OrderBook book;

    EXPECT_EQ(book.orderCount(), 0U);
    EXPECT_EQ(book.orderLookupSize(), 0U);
}

TEST(OrderBookTest, OrderCountCountsRestingOrdersOnBothSides)
{
    OrderBook book;
    addGtc(book, makeOrderId(1), Side::BUY, 100, 5);
    addGtc(book, makeOrderId(2), Side::BUY, 100, 5);
    addGtc(book, makeOrderId(3), Side::BUY, 99, 5);
    addGtc(book, makeOrderId(4), Side::SELL, 101, 5);

    EXPECT_EQ(book.orderCount(), 4U);
    EXPECT_EQ(book.orderLookupSize(), 4U);
}

TEST(OrderBookTest, OrderCountExcludesFilledAndNonRestingOrders)
{
    OrderBook book;
    addGtc(book, makeOrderId(1), Side::SELL, 100, 5);
    addGtc(book, makeOrderId(2), Side::SELL, 101, 5);

    book.addOrder(makeOrderId(3), Side::BUY, Price{100}, Quantity{5}, Quantity{0}, TimeInForce::IOC, OrderType::LIMIT);
    book.addOrder(makeOrderId(4), Side::BUY, std::nullopt, Quantity{2}, Quantity{0}, TimeInForce::IOC,
                  OrderType::MARKET);

    EXPECT_EQ(book.orderCount(), 1U);
    EXPECT_EQ(book.orderLookupSize(), 1U);
}

TEST(OrderBookTest, OrderLookupSizeMatchesOrderCountThroughEveryOperation)
{
    OrderBook book;
    const auto expectConsistent = [&book](const std::size_t expected) {
        EXPECT_EQ(book.orderCount(), expected);
        EXPECT_EQ(book.orderLookupSize(), expected);
    };
    const auto first = makeOrderId(1);
    const auto second = makeOrderId(2);
    const auto third = makeOrderId(3);

    addGtc(book, first, Side::SELL, 100, 10);
    addGtc(book, second, Side::SELL, 100, 10);
    addGtc(book, third, Side::BUY, 98, 10);
    expectConsistent(3);

    book.amendOrder(first, Quantity{15});
    expectConsistent(3);

    book.amendOrder(second, Price{99});
    expectConsistent(3);

    book.amendOrder(third, Quantity{12}, Price{99});
    expectConsistent(2);

    book.amendOrder(first, Quantity{0});
    expectConsistent(1);

    book.removeOrder(third);
    expectConsistent(0);

    book.removeOrder(third);
    expectConsistent(0);
}

TEST(OrderBookTest, GetOrderReturnsNothingForAnUnknownOrder)
{
    OrderBook book;
    addGtc(book, makeOrderId(1), Side::BUY, 100, 5);

    EXPECT_FALSE(book.getOrder(makeOrderId(2)).has_value());
}

TEST(OrderBookTest, GetOrderReturnsARestingOrderOnEitherSide)
{
    OrderBook book;
    const auto buy = makeOrderId(1);
    const auto sell = makeOrderId(2);
    addGtc(book, buy, Side::BUY, 99, 5);
    addGtc(book, sell, Side::SELL, 101, 7);

    const auto buyOrder = book.getOrder(buy);
    ASSERT_TRUE(buyOrder.has_value());
    EXPECT_EQ(buyOrder->id, buy);
    EXPECT_EQ(buyOrder->price, Price{99});
    EXPECT_EQ(buyOrder->totalQuantity, Quantity{5});
    EXPECT_EQ(buyOrder->filledQuantity, Quantity{0});
    EXPECT_EQ(buyOrder->side, Side::BUY);

    const auto sellOrder = book.getOrder(sell);
    ASSERT_TRUE(sellOrder.has_value());
    EXPECT_EQ(sellOrder->price, Price{101});
    EXPECT_EQ(sellOrder->side, Side::SELL);
}

TEST(OrderBookTest, GetOrderReflectsFillsAndAmendments)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::SELL, 100, 10);
    addGtc(book, makeOrderId(2), Side::BUY, 100, 4);
    book.amendOrder(resting, Quantity{12}, Price{102});

    const auto order = book.getOrder(resting);

    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->price, Price{102});
    EXPECT_EQ(order->totalQuantity, Quantity{12});
    EXPECT_EQ(order->filledQuantity, Quantity{4});
}

TEST(OrderBookTest, GetOrderReturnsNothingOnceTheOrderLeavesTheBook)
{
    OrderBook book;
    const auto filled = makeOrderId(1);
    const auto cancelled = makeOrderId(2);
    addGtc(book, filled, Side::SELL, 100, 5);
    addGtc(book, cancelled, Side::SELL, 101, 5);

    addGtc(book, makeOrderId(3), Side::BUY, 100, 5);
    book.removeOrder(cancelled);

    EXPECT_FALSE(book.getOrder(filled).has_value());
    EXPECT_FALSE(book.getOrder(cancelled).has_value());
}

TEST(OrderBookTest, GetOrderReturnsACopyThatOutlivesTheOrder)
{
    OrderBook book;
    const auto resting = makeOrderId(1);
    addGtc(book, resting, Side::BUY, 100, 5);

    const auto order = book.getOrder(resting);
    book.removeOrder(resting);
    addGtc(book, makeOrderId(2), Side::BUY, 50, 9);

    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->price, Price{100});
    EXPECT_EQ(order->totalQuantity, Quantity{5});
}

} // namespace exchange::server
