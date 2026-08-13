#pragma once

#include "server/common/model/event/event.hpp"

namespace exchange::server::common::model
{
struct CancelLimitOrderEvent : Event
{

    CancelLimitOrderEvent(const OrderId orderId, const AccountId accountId) : Event(orderId, accountId)
    {
    }

    [[nodiscard]] EventType getType() const override
    {
        return EventType::CANCEL_LIMIT_ORDER;
    }
};
} // namespace exchange::server::common::model
