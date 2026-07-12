#pragma once
#include "securities.pb.h"

#include <memory>

namespace exchange::common::model
{
using namespace v1;

class Universe
{
  public:
    static constexpr size_t NUMBER_OF_SECURITIES = 3;

  private:
    static std::shared_ptr<std::array<Security, NUMBER_OF_SECURITIES>> securities;

  public:
    static std::shared_ptr<const std::array<Security, NUMBER_OF_SECURITIES>> getSecurities();
};
} // namespace exchange::common::model