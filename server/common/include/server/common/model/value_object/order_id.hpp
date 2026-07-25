#pragma once
#include <functional>
#include <cstdint>

namespace exchange::server::common::model
{
struct OrderId
{
    uint64_t value;
    explicit OrderId(const uint64_t value) : value(value)
    {
    }

    bool operator==(const OrderId &other) const
    {
        return value == other.value;
    }
};
} // namespace exchange::server::common::model

template <> struct std::hash<exchange::server::common::model::OrderId>
{
    std::size_t operator()(const exchange::server::common::model::OrderId &id) const noexcept
    {
        return std::hash<uint64_t>{}(id.value);
    }
};