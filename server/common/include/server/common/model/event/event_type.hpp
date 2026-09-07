#pragma once
#include <cstdint>

namespace exchange::server
{
    enum class EventType : uint8_t
    {
        AMEND_LIMIT_ORDER,
        CANCEL_LIMIT_ORDER,
        NEW_LIMIT_ORDER,
        NEW_MARKET_ORDER,
    };
}
