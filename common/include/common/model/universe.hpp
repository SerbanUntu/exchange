#pragma once
#include "common/proto/proto_declarations.hpp"

namespace exchange::common
{
class Universe
{
  public:
    static constexpr size_t NUMBER_OF_SECURITIES = 3;
    static const std::array<proto::Security, NUMBER_OF_SECURITIES> &getSecurities();
};
} // namespace exchange::common