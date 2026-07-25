#pragma once

#include <cstdint>
namespace exchange::server::common::model
{
struct AccountId
{
    uint64_t value;
    explicit AccountId(const uint64_t value) : value(value)
    {
    }
};
} // namespace exchange::server::common::model
