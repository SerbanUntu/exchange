#pragma once
#include "matched_order.hpp"
#include "order_node.hpp"
#include "price_node.hpp"
#include "server/common/model/side.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/price.hpp"

#include <memory>
#include <set>
#include <unordered_map>

namespace exchange::server
{
class OrderBook
{
    std::unordered_map<Price, std::unique_ptr<PriceNode>> buyLevels;
    std::unordered_map<Price, std::unique_ptr<PriceNode>> sellLevels;
    std::unordered_map<OrderId, std::unique_ptr<OrderNode>> orderMap;
    std::set<Price> buyPriceSet;
    std::set<Price> sellPriceSet;

  public:
    void addOrder(OrderId orderId, AccountId accountId, Side side, Price price, Quantity quantity,
                  TimeInForce timeInForce) noexcept;
    bool removeOrder(OrderId orderId, AccountId accountId) noexcept;
    bool amendOrder(OrderId orderId, AccountId accountId, Quantity newQuantity) noexcept;
    bool amendOrder(OrderId orderId, AccountId accountId, Price price) noexcept;
    bool amendOrder(OrderId orderId, AccountId accountId, Quantity newQuantity,
                    Price price) noexcept;
    bool partiallyFillOrder(OrderId orderId, AccountId accountId, Quantity filledQuantity) noexcept;

    /**
     *
     * @param side The side of the order book to match against (BUY or SELL)
     * @param price The price to match against (accepts lower prices on the SELL book and higher prices on the BUY book)
     * @param quantity The quantity that can be matched
     * @param allOrNothing If true, no orders will be matched if the quantity cannot be fully matched
     * @return A pair of matched orders and the remaining quantity that could not be matched
     */
    std::pair<std::vector<MatchedOrder>, Quantity> matchOrders(Side side, Price price,
                                                              Quantity quantity, bool allOrNothing) noexcept;
};
} // namespace exchange::server