#pragma once
#include <cstdint>

namespace exchange::server::common::model
{
enum class OrderType : uint8_t
{
    LIMIT,
    MARKET,
};
} // namespace exchange::server::common::model