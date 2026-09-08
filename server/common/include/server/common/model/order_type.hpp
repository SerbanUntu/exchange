#pragma once
#include <cstdint>

namespace exchange::server
{
enum class OrderType : uint8_t
{
    LIMIT,
    MARKET,
};
} // namespace exchange::server