#pragma once
#include <functional>
#include <cstdint>

namespace exchange::server
{
struct Price
{
    uint64_t value{};
    constexpr Price() = default;
    constexpr explicit Price(const uint64_t value) : value(value)
    {
    }

    constexpr auto operator<=>(const Price &) const = default;
};
} // namespace exchange::server

template <> struct std::hash<exchange::server::Price>
{
    std::size_t operator()(const exchange::server::Price &price) const noexcept
    {
        return std::hash<uint64_t>{}(price.value);
    }
};

template <> class std::numeric_limits<exchange::server::Price> : public std::numeric_limits<uint64_t>
{
  public:
    static constexpr bool is_specialized = true;

    static constexpr exchange::server::Price max() noexcept
    {
        return exchange::server::Price{std::numeric_limits<uint64_t>::max()};
    }

    static constexpr exchange::server::Price min() noexcept
    {
        return exchange::server::Price{std::numeric_limits<uint64_t>::min()};
    }
};
