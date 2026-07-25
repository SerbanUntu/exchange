#pragma once

#include "server/common/model/side.hpp"
#include "server/common/model/event/event.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server::common::model
{
struct NewMarketOrderEvent : Event
{
    const Side side;
    const Quantity quantity;

    NewMarketOrderEvent(const OrderId orderId, const AccountId accountId, const Side side, const Quantity quantity)
        : Event(orderId, accountId), side(side), quantity(quantity)
    {
    }
};
} // namespace exchange::server::common::model
