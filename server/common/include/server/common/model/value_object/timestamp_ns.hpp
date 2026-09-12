#pragma once
#include <chrono>

namespace exchange::server
{
using TimestampNs = std::chrono::sys_time<std::chrono::nanoseconds>;
} // namespace exchange::server