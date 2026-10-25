#pragma once

#include <cstdint>
namespace exchange::server
{
struct AccountId
{
    uint64_t value{};
    constexpr AccountId() = default;
    explicit AccountId(const uint64_t value) : value(value)
    {
    }
};
} // namespace exchange::server
