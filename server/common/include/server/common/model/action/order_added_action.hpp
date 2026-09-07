#pragma once
#include "action.hpp"
#include "server/common/model/time_in_force.hpp"
#include "server/common/model/value_object/account_id.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct OrderAddedAction : Action
{
    const Price price;
    const Quantity quantity;
    const TimeInForce timeInForce;
    OrderAddedAction(const OrderId orderId, const AccountId accountId, const Price price, const Quantity quantity,
                     const TimeInForce timeInForce)
        : Action(orderId, accountId), price(price), quantity(quantity), timeInForce(timeInForce)
    {
    }

    [[nodiscard]] ActionType getType() const override
    {
        return ActionType::ORDER_ADDED;
    }
};
} // namespace exchange::server