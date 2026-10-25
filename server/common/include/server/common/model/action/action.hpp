#pragma once
#include "server/common/model/action/order_added_action.hpp"
#include "server/common/model/action/order_downsized_action.hpp"
#include "server/common/model/action/order_removed_action.hpp"
#include "server/common/model/action/order_upsized_action.hpp"
#include "server/common/model/action/trade_action.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/security_id.hpp"
#include "server/common/model/value_object/timestamp_ns.hpp"

#include <type_traits>
#include <variant>

namespace exchange::server
{
struct alignas(64) Action
{
    struct Header
    {
        OrderId orderId;
        SecurityId securityId{0};
        TimestampNs timestamp;
    };
    Header header;
    std::variant<OrderAddedAction, OrderDownsizedAction, OrderRemovedAction, OrderUpsizedAction, TradeAction> body;
};
static_assert(std::is_trivially_copyable_v<Action>);
static_assert(std::is_default_constructible_v<Action>);
static_assert(std::is_copy_assignable_v<Action>);
} // namespace exchange::server
