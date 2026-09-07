#pragma once

#include "event_type.hpp"
#include "server/common/model/value_object/account_id.hpp"
#include "server/common/model/value_object/order_id.hpp"

namespace exchange::server
{
struct Event
{
    const OrderId orderId;
    const AccountId accountId;

    virtual ~Event() = default;
    [[nodiscard]] virtual EventType getType() const = 0;

  protected:
    Event(const OrderId orderId, const AccountId accountId)
        : orderId(orderId), accountId(accountId)
    {
    }
};
} // namespace exchange::server
