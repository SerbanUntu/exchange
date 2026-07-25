#pragma once
#include <functional>
#include <cstdint>

namespace exchange::server::common::model
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
} // namespace exchange::server::common::model

template <> struct std::hash<exchange::server::common::model::Price>
{
    std::size_t operator()(const exchange::server::common::model::Price &price) const noexcept
    {
        return std::hash<uint64_t>{}(price.value);
    }
};