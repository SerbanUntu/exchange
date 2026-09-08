#pragma once
#include "action_type.hpp"
#include "server/common/model/value_object/order_id.hpp"

namespace exchange::server
{
struct Action
{
    const OrderId orderId;

    virtual ~Action() = default;
    [[nodiscard]] virtual ActionType getType() const = 0;

  protected:
    explicit Action(const OrderId orderId) : orderId(orderId)
    {
    }
};
} // namespace exchange::server
