#pragma once
#include "server/common/model/action/action.hpp"

namespace exchange::server
{
struct OrderRemovedAction : Action
{
    explicit OrderRemovedAction(const OrderId orderId) : Action(orderId)
    {
    }

    [[nodiscard]] ActionType getType() const override
    {
        return ActionType::ORDER_REMOVED;
    }
};
} // namespace exchange::server