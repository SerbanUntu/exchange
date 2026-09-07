#pragma once
#include <functional>
#include <cstdint>

namespace exchange::server
{
struct Price
{
    uint64_t value;
    explicit Price(const uint64_t value) : value(value)
    {
    }

    bool operator==(const Price &other) const
    {
        return value == other.value;
    }

    bool operator<(const Price &other) const
    {
        return value < other.value;

    }
};
} // namespace exchange::server

template <> struct std::hash<exchange::server::Price>
{
    std::size_t operator()(const exchange::server::Price &price) const noexcept
    {
        return std::hash<uint64_t>{}(price.value);
    }
};