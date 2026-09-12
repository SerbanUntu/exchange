#pragma once

#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"
#include <chrono>

namespace exchange::server
{
struct Candle
{
    Price open{0};
    Price high{std::numeric_limits<Price>::min()};
    Price low{std::numeric_limits<Price>::max()};
    Price close{0};
    Quantity volume{0};
    std::chrono::sys_seconds start{};
    std::chrono::seconds resolution{std::chrono::seconds{1}};
    bool isClosed{false};

    bool empty() const
    {
        return volume == Quantity{0};
    }
};
} // namespace exchange::server
