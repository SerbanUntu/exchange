#pragma once
#include "server/common/model/side.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct OrderAddedAction
{
    Price price;
    Quantity totalQuantity;
    Quantity filledQuantity;
    Side side;
};
} // namespace exchange::server
