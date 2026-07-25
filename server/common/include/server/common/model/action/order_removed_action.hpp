#pragma once
#include "action.hpp"

namespace exchange::server::common::model
{
struct OrderRemovedAction : Action
{
    OrderRemovedAction(const OrderId orderId, const AccountId accountId) : Action(orderId, accountId)
    {
    }
};
} // namespace exchange::server::common::model