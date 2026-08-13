#pragma once
#include "server/common/model/action/action.hpp"

namespace exchange::server::common::model
{
struct OrderCancelledAction : Action
{
    OrderCancelledAction(const OrderId orderId, const AccountId accountId) : Action(orderId, accountId)
    {
    }

    [[nodiscard]] ActionType getType() const override
    {
        return ActionType::ORDER_CANCELLED;
    }
};
} // namespace exchange::server::common::model