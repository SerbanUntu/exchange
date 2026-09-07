#pragma once
#include <cstdint>

namespace exchange::server
{
    enum class ActionType : uint8_t
    {
        ORDER_ADDED,
        ORDER_AMENDED,
        ORDER_CANCELLED,
        ORDER_EXECUTED,
        ORDER_MATCHED,
    };
}