#pragma once
#include <cstdint>

namespace exchange::server::common::model
{
enum class Side : uint8_t
{
    BUY,
    SELL,
};
} // namespace exchange::server::common::model