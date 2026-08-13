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

namespace exchange::server::common::collections
{
class OrderBook
{
    std::unordered_map<model::Price, std::unique_ptr<PriceNode>> buyLevels;
    std::unordered_map<model::Price, std::unique_ptr<PriceNode>> sellLevels;
    std::unordered_map<model::OrderId, std::unique_ptr<OrderNode>> orderMap;
    std::set<model::Price> buyPriceSet;
    std::set<model::Price> sellPriceSet;

  public:
    void addOrder(model::OrderId orderId, model::AccountId accountId, model::Side side, model::Price price, model::Quantity quantity,
                  model::TimeInForce timeInForce) noexcept;
    bool removeOrder(model::OrderId orderId, model::AccountId accountId) noexcept;
    bool amendOrder(model::OrderId orderId, model::AccountId accountId, model::Quantity newQuantity) noexcept;
    bool amendOrder(model::OrderId orderId, model::AccountId accountId, model::Price price) noexcept;
    bool amendOrder(model::OrderId orderId, model::AccountId accountId, model::Quantity newQuantity,
                    model::Price price) noexcept;
    bool partiallyFillOrder(model::OrderId orderId, model::AccountId accountId, model::Quantity filledQuantity) noexcept;

    /**
     *
     * @param side The side of the order book to match against (BUY or SELL)
     * @param price The price to match against (accepts lower prices on the SELL book and higher prices on the BUY book)
     * @param quantity The quantity that can be matched
     * @param allOrNothing If true, no orders will be matched if the quantity cannot be fully matched
     * @return A pair of matched orders and the remaining quantity that could not be matched
     */
    std::pair<std::vector<MatchedOrder>, model::Quantity> matchOrders(model::Side side, model::Price price,
                                                                      model::Quantity quantity, bool allOrNothing) noexcept;
};
} // namespace exchange::server::common::collections