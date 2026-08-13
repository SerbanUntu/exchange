#pragma once
#include "server/common/model/action/action.hpp"

namespace exchange::server::common::model
{
struct OrderExecutedAction : Action
{
    OrderExecutedAction(const OrderId orderId, const AccountId accountId) : Action(orderId, accountId)
    {
    }

    [[nodiscard]] ActionType getType() const override
    {
        return ActionType::ORDER_EXECUTED;
    }
};
} // namespace exchange::server::common::model
