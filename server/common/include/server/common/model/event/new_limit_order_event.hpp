#pragma once

#include "server/common/model/side.hpp"
#include "server/common/model/time_in_force.hpp"
#include "server/common/model/event/event.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server::common::model
{
struct NewLimitOrderEvent : Event
{
    const Side side;
    const Quantity quantity;
    const Price price;
    const TimeInForce tif;

    NewLimitOrderEvent(const OrderId orderId, const AccountId accountId, const Side side, const Quantity quantity,
                       const Price price, const TimeInForce tif)
        : Event(orderId, accountId), side(side), quantity(quantity), price(price), tif(tif)
    {
    }

    [[nodiscard]] EventType getType() const override
    {
        return EventType::NEW_LIMIT_ORDER;
    }
};
} // namespace exchange::server::common::model
