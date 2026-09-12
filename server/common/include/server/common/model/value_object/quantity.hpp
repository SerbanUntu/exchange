#pragma once

#include <cstdint>
#include <compare>
#include <limits>
namespace exchange::server
{
struct Quantity
{
    uint64_t value{};
    constexpr Quantity() = default;
    constexpr explicit Quantity(const uint64_t value) : value(value)
    {
    }

    auto operator<=>(const Quantity &other) const = default;

    constexpr auto operator-(const Quantity &other) const
    {
        return Quantity(value - other.value);
    }
    constexpr void operator+=(const Quantity &other)
    {
        value += other.value;
    }
    constexpr void operator-=(const Quantity &other)
    {
        value -= other.value;
    }
};
} // namespace exchange::server

template <> class std::numeric_limits<exchange::server::Quantity> : public std::numeric_limits<uint64_t>
{
  public:
    static constexpr bool is_specialized = true;

    static constexpr exchange::server::Quantity max() noexcept
    {
        return exchange::server::Quantity{std::numeric_limits<uint64_t>::max()};
    }

    static constexpr exchange::server::Quantity min() noexcept
    {
        return exchange::server::Quantity{std::numeric_limits<uint64_t>::min()};
    }
};