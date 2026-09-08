#pragma once
#include "server/common/model/side.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{

/**
 * An order resting inside an orderbook. Only applicable to GTC limit orders.
 */
struct Order
{
    const OrderId id;
    Price price;
    Quantity totalQuantity;
    Quantity filledQuantity;
    Side side;

    Quantity remainingQuantity() const noexcept
    {
        return totalQuantity - filledQuantity;
    }
};
} // namespace exchange::server