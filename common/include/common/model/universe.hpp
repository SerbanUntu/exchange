#pragma once
#include "securities.pb.h"

namespace exchange::common
{
using namespace v1;

class Universe
{
  public:
    static constexpr size_t NUMBER_OF_SECURITIES = 3;
    static const std::array<Security, NUMBER_OF_SECURITIES> &getSecurities();
};
} // namespace exchange::common