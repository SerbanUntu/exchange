#pragma once

#include "server/common/model/event/event.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

#include <optional>

namespace exchange::server
{
struct AmendLimitOrderEvent : Event
{
    const std::optional<Quantity> quantity;
    const std::optional<Price> price;

    AmendLimitOrderEvent(const OrderId orderId, const AccountId accountId, const std::optional<Quantity> quantity,
                         const std::optional<Price> price)
        : Event(orderId, accountId), quantity(quantity), price(price)
    {
    }

    [[nodiscard]] EventType getType() const override
    {
        return EventType::AMEND_LIMIT_ORDER;
    }
};
} // namespace exchange::server
