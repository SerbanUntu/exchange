#pragma once
#include "server/common/model/side.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct TradeAction
{
    OrderId restingId;
    Price price;
    Quantity quantity;
    Side aggressorSide;
};
} // namespace exchange::server
