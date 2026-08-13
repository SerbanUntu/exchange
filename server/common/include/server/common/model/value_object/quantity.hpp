#pragma once

#include <cstdint>
#include <compare>
namespace exchange::server::common::model
{
struct Quantity
{
    uint64_t value;
    explicit Quantity(const uint64_t value) : value(value)
    {
    }

    auto operator<=>(const Quantity& other) const = default;

    auto operator-(const Quantity& other) const
    {
        return Quantity(value - other.value);
    }
};
} // namespace exchange::server::common::model
