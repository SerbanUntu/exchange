#pragma once
#include <cstdint>

namespace exchange::server
{
enum class Side : uint8_t
{
    BUY,
    SELL,
};
} // namespace exchange::server