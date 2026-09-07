#pragma once
#include <functional>
#include <cstdint>

namespace exchange::server
{
struct SecurityId
{
    uint32_t value;
    explicit SecurityId(const uint32_t value) : value(value)
    {
    }

    bool operator==(const SecurityId &other) const
    {
        return value == other.value;
    }
};
} // namespace exchange::server

template <> struct std::hash<exchange::server::SecurityId>
{
    std::size_t operator()(const exchange::server::SecurityId &id) const noexcept
    {
        return std::hash<uint32_t>{}(id.value);
    }
};