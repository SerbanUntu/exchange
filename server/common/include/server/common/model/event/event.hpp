#pragma once

#include "amend_limit_order_event.hpp"
#include "cancel_limit_order_event.hpp"
#include "new_limit_order_event.hpp"
#include "new_market_order_event.hpp"
#include "server/common/model/value_object/account_id.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/security_id.hpp"

#include <type_traits>
#include <variant>

namespace exchange::server
{
struct alignas(64) Event
{
    struct Header
    {
        OrderId orderId;
        SecurityId securityId;
        AccountId accountId;
    };
    Header header;
    std::variant<AmendLimitOrderEvent, CancelLimitOrderEvent, NewLimitOrderEvent, NewMarketOrderEvent> body;
};
static_assert(std::is_trivially_copyable_v<Event>);
static_assert(std::is_default_constructible_v<Event>);
static_assert(std::is_copy_assignable_v<Event>);
} // namespace exchange::server
