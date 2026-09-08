#pragma once
#include <cstdint>

namespace exchange::server
{
enum class ActionType : uint8_t
{
    ORDER_ADDED,
    ORDER_DOWNSIZED,
    ORDER_REMOVED,
    ORDER_UPSIZED,
    TRADE,
};
} // namespace exchange::server