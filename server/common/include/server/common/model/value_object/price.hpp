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

    auto operator<=>(const Price &) const = default;
};
} // namespace exchange::server

template <> struct std::hash<exchange::server::Price>
{
    std::size_t operator()(const exchange::server::Price &price) const noexcept
    {
        return std::hash<uint64_t>{}(price.value);
    }
};