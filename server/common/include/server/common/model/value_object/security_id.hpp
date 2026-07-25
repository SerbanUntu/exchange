#pragma once
#include <cstdint>

namespace exchange::server::common::model
{
struct SecurityId
{
    uint32_t value;
    explicit SecurityId(const uint32_t value) : value(value)
    {
    }
};
} // namespace exchange::server::common::model
