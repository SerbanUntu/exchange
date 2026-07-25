#pragma once

#include "server/common/model/event/event.hpp"

namespace exchange::server::common::model
{
struct CancelLimitOrderEvent : Event
{

    CancelLimitOrderEvent(const OrderId orderId, const AccountId accountId) : Event(orderId, accountId)
    {
    }
};
} // namespace exchange::server::common::model
