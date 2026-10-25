#pragma once

#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct AmendLimitOrderEvent
{
    Quantity quantity;
    Price price;
    bool isQuantityAmended;
    bool isPriceAmended;
};
} // namespace exchange::server
