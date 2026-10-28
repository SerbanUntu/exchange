#pragma once
#include "server/common/collections/orderbook/order.hpp"
#include "server/common/collections/orderbook/orderbook.hpp"
#include "server/common/model/action/action.hpp"
#include "server/common/model/event/event.hpp"
#include "server/common/model/order_type.hpp"
#include "server/common/model/side.hpp"
#include "server/common/model/time_in_force.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"
#include "server/common/model/value_object/security_id.hpp"

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <vector>

namespace exchange::server
{

class ReferenceOrderBook
{
    std::vector<Order> orders;

    [[nodiscard]] std::optional<std::size_t> indexOf(OrderId orderId) const;
    [[nodiscard]] std::optional<std::size_t> bestMatch(Side side, std::optional<Price> limit) const;
    [[nodiscard]] Quantity availableQuantity(Side side, std::optional<Price> limit) const;
    void erase(OrderId orderId);

  public:
    OrderBook::AddOrderResult addOrder(OrderId orderId, Side side, std::optional<Price> price, Quantity totalQuantity,
                                       Quantity filledQuantity, TimeInForce timeInForce, OrderType orderType);
    bool removeOrder(OrderId orderId);
    OrderBook::AmendOrderStatus amendOrder(OrderId orderId, Quantity newQuantity);
    OrderBook::AmendOrderResult amendOrder(OrderId orderId, Price newPrice);
    OrderBook::AmendOrderResult amendOrder(OrderId orderId, Quantity newQuantity, Price newPrice);

    [[nodiscard]] std::optional<Order> getOrder(OrderId orderId) const;
    [[nodiscard]] std::size_t orderCount() const;
};

std::vector<Action> referenceProcessEvent(const Event &event,
                                          std::unordered_map<SecurityId, ReferenceOrderBook> &state);

} // namespace exchange::server
