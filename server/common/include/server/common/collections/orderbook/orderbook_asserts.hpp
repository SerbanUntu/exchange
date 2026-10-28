#pragma once
#include "orderbook.hpp"
#include "server/common/model/side.hpp"

#include <utility>
#include <vector>

namespace exchange::server
{

/**
 * Calls `assert()` to check for these invariants:
 * - The best buy price is less than the best sell price.
 * - The size of the order lookup table is equal to the number of orders.
 *
 * @param orderBook The orderbook to perform the assertions on.
 * @param onlyOrderBookAsserts If true, it only performs the aforementioned asserts.
 * Otherwise, it also runs the asserts to check the invariants of every resting order,
 * described by `assertRestingOrderInvariants`.
 */
void assertOrderBookInvariants(const OrderBook &orderBook, bool onlyOrderBookAsserts = false);

/**
 * Calls `assert()` to check for these invariants:
 * - Every resting order has less filled quantity than total quantity.
 * - Every resting order is resting in the correct book and at the correct price level.
 *
 * @param order The order to perform the assertions on.
 * @param bookSide The actual book the order rests on (buy book or sell book).
 * @param priceLevel The price level in the orderbook at which the order rests.
 */
void assertRestingOrderInvariants(const Order &order, Side bookSide, Price priceLevel);

/**
 * Calls `assert()` to check for these invariants:
 * - Trade prices are non-decreasing for buys and non-increasing for sells, and are never worse than the price of the
 * aggressor.
 * - The quantity of trades is less than or equal to the remaining quantity of the aggressor (before any trades).
 * - The quantity of trades is equal to the increase in the filledQuantity of the aggressor.
 * - Every trade executes at the resting order's price.
 * - At most one resting order ends partially filled, and its trade is the last one in the list.
 *
 * @param aggressor The newly arrived order that matches with resting orders (before any trades are performed).
 * @param aggressorFilledQuantity How much the filled quantity of the aggressor is after all trades are performed.
 * @param tradeRestingPairs Pairs of the trade and the corresponding matching order (before the trade is made).
 */
void assertMatchingInvariants(const Order &aggressor, Quantity aggressorFilledQuantity,
                              std::vector<std::pair<Trade, Order>> tradeRestingPairs);

} // namespace exchange::server