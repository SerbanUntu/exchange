#pragma once
#include "order.hpp"
#include "trade.hpp"
#include "server/common/model/order_type.hpp"
#include "server/common/model/side.hpp"
#include "server/common/model/time_in_force.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/price.hpp"

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace exchange::server
{
using OrderBookMap = std::map<Price, std::list<Order>, std::function<bool(const Price &, const Price &)>>;
using OrderLookupValue = std::pair<std::list<Order> *, std::list<Order>::iterator>;

class OrderBook
{
    OrderBookMap buyBook{std::greater{}};
    OrderBookMap sellBook{std::less{}};
    std::unordered_map<OrderId, OrderLookupValue> orderLookup;

  public:
    OrderBook() = default;
    OrderBook(const OrderBook &) = delete; // Would copy the lookup pointers causing errors
    OrderBook &operator=(const OrderBook &) = delete;
    OrderBook(OrderBook &&) noexcept = default;
    OrderBook &operator=(OrderBook &&) noexcept = default;

    enum class AmendOrderStatus : uint8_t
    {
        CANNOT_AMEND,
        AMENDED_IN_PLACE,
        AMENDED,
        REMOVED
    };
    struct AddOrderResult
    {
        std::optional<Order> addedOrder;
        std::vector<Trade> trades;
    };
    struct AmendOrderResult
    {
        AmendOrderStatus status;
        AddOrderResult tradesAfterLosingPriority;
    };
    /**
     * Try to match an order to what currently exists on the book, if possible,
     * and then potentially add it to the book in the case of GTC limit orders.
     *
     * @param orderId The id of the incoming order
     * @param side The side of the incoming order (BUY or SELL)
     * @param price The price of the incoming order, if it is a limit order
     * @param quantity The quantity of the incoming order
     * @param timeInForce The time in force of the incoming order
     * @param orderType Whether the incoming order is a limit order or a market order
     *
     * @return A list of matches performed between the incoming order and orders resting on the book, if any
     */
    AddOrderResult addOrder(OrderId orderId, Side side, std::optional<Price> price, Quantity quantity,
                            TimeInForce timeInForce, OrderType orderType) noexcept;

    /**
     * Remove a GTC limit order that is currently resting on the book.
     *
     * @param orderId The id of the order to remove
     * @return True if the order is resting on the book, false otherwise
     */
    bool removeOrder(OrderId orderId) noexcept;

    /**
     * Modify the quantity of a GTC limit order that is currently resting on the book.
     * Specifying a quantity less than the original one does not reset time priority.
     *
     * @param orderId The id of the order to amend
     * @param newQuantity The new total quantity of the order
     */
    AmendOrderStatus amendOrder(OrderId orderId, Quantity newQuantity) noexcept;

    /**
     * Modify the price of a GTC limit order that is currently resting on the book.
     * Always resets time priority.
     *
     * @param orderId The id of the order to amend
     * @param newPrice The new price of the order
     */
    AmendOrderResult amendOrder(OrderId orderId, Price newPrice) noexcept;

    /**
     * Modify the quantity and price of a GTC limit order that is currently resting on the book.
     * Resets time priority, unless the price is unchanged and the quantity only decreases,
     * which is treated the same as amending the quantity on its own.
     *
     * @param orderId The id of the order to amend
     * @param newQuantity The new total quantity of the order
     * @param newPrice The new price of the order
     */
    AmendOrderResult amendOrder(OrderId orderId, Quantity newQuantity, Price newPrice) noexcept;
};
} // namespace exchange::server