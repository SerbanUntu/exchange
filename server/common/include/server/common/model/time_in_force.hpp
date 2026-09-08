#pragma once
#include <cstdint>

namespace exchange::server
{
enum class TimeInForce : uint8_t
{
    GTC,
    IOC,
    FOK
};
} // namespace exchange::server