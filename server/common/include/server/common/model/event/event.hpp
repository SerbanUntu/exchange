#pragma once

#include "event_type.hpp"
#include "server/common/model/value_object/order_id.hpp"

namespace exchange::server
{
struct Event
{
    const OrderId orderId;

    virtual ~Event() = default;
    [[nodiscard]] virtual EventType getType() const = 0;

  protected:
    explicit Event(const OrderId orderId) : orderId(orderId)
    {
    }
};
} // namespace exchange::server
