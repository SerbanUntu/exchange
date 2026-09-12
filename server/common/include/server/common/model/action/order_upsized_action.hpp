#pragma once
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct OrderUpsizedAction
{
    Quantity newQuantity;
};
} // namespace exchange::server
