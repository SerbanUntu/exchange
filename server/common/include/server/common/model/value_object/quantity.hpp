#pragma once

#include <cstdint>
namespace exchange::server::common::model
{
struct Quantity
{
    uint64_t value;
    explicit Quantity(const uint64_t value) : value(value)
    {
    }
};
} // namespace exchange::server::common::model
