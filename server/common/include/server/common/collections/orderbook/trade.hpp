#pragma once
#include "server/common/model/side.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct Trade
{
    enum class TradeResult : uint8_t
    {
        AGGRESSOR_FILLED,
        RESTING_FILLED,
        BOTH
    };
    const OrderId aggressorId;
    const OrderId restingId;
    const Price price;
    const Quantity quantity;
    const Side aggressorSide;
    const TradeResult result;
};
} // namespace exchange::server