#pragma once

#include "server/common/model/side.hpp"
#include "server/common/model/time_in_force.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct NewLimitOrderEvent
{
    Side side;
    Quantity quantity;
    Price price;
    TimeInForce tif;
};
} // namespace exchange::server
