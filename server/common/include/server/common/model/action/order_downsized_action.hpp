#pragma once
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct OrderDownsizedAction
{
    Quantity newQuantity;
};
} // namespace exchange::server
