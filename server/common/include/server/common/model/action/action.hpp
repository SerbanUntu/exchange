#pragma once
#include "action_type.hpp"
#include "server/common/model/value_object/account_id.hpp"
#include "server/common/model/value_object/order_id.hpp"

namespace exchange::server
{
struct Action
{
    const OrderId orderId;
    const AccountId accountId;

    virtual ~Action() = default;
    [[nodiscard]] virtual ActionType getType() const = 0;

protected:
    Action(const OrderId orderId, const AccountId accountId)
        : orderId(orderId), accountId(accountId)
    {
    }
};
} // namespace exchange::server
