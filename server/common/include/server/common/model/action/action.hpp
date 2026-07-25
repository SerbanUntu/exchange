#pragma once
#include "server/common/model/value_object/account_id.hpp"
#include "server/common/model/value_object/order_id.hpp"

namespace exchange::server::common::model
{
struct Action
{
    const OrderId orderId;
    const AccountId accountId;

    virtual ~Action() = default;

protected:
    Action(const OrderId orderId, const AccountId accountId)
        : orderId(orderId), accountId(accountId)
    {
    }
};
} // namespace exchange::server::common::model
