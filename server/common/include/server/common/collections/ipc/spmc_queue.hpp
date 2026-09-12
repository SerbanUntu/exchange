#pragma once
#include <common/util/concepts/trivially_copyable.hpp>

namespace exchange::server
{

template <TriviallyCopyable T> class SPMCQueue
{
  public:
    T pop() noexcept; // blocking
    void push(const T &item) noexcept;
};

} // namespace exchange::server