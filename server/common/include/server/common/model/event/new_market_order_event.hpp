#pragma once

#include "server/common/model/side.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct NewMarketOrderEvent
{
    Side side;
    Quantity quantity;
};
} // namespace exchange::server
