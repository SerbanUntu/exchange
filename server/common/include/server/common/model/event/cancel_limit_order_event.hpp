#pragma once

#include "server/common/model/event/event.hpp"

namespace exchange::server
{
struct CancelLimitOrderEvent : Event
{
    CancelLimitOrderEvent(const OrderId orderId) : Event(orderId)
    {
    }

    [[nodiscard]] EventType getType() const override
    {
        return EventType::CANCEL_LIMIT_ORDER;
    }
};
} // namespace exchange::server
