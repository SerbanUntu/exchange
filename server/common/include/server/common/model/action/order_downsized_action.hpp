#pragma once
#include "server/common/model/action/action.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct OrderDownsizedAction : Action
{
    const Quantity newQuantity;

    OrderDownsizedAction(const OrderId orderId, const Quantity newQuantity) : Action(orderId), newQuantity(newQuantity)
    {
    }

    [[nodiscard]] ActionType getType() const override
    {
        return ActionType::ORDER_DOWNSIZED;
    }
};
} // namespace exchange::server
