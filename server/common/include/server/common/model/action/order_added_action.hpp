#pragma once
#include "action.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct OrderAddedAction : Action
{
    const Price price;
    const Quantity totalQuantity;
    const Quantity filledQuantity;
    const Side side;

    OrderAddedAction(const OrderId orderId, const Price price, const Quantity totalQuantity,
                     const Quantity filledQuantity, const Side side)
        : Action(orderId), price(price), totalQuantity(totalQuantity), filledQuantity(filledQuantity), side(side)
    {
    }

    [[nodiscard]] ActionType getType() const override
    {
        return ActionType::ORDER_ADDED;
    }
};
} // namespace exchange::server