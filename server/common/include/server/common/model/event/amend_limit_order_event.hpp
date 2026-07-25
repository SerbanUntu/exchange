#pragma once

#include "server/common/model/event/event.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

#include <optional>

namespace exchange::server::common::model
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
};
} // namespace exchange::server::common::model
