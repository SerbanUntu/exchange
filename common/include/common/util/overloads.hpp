#pragma once

namespace exchange
{
template <typename... Ts> struct overloads : Ts...
{
    using Ts::operator()...;
};
}; // namespace exchange