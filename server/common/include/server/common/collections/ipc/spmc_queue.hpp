#pragma once
#include <common/util/concepts/trivially_copyable.hpp>

#include <queue>

namespace exchange::server
{

template <TriviallyCopyable T> class SPMCQueue
{
    std::queue<T> q; // mock implementation
  public:
    [[nodiscard]] T pop() noexcept; // blocking
    void push(const T &item) noexcept;
};

} // namespace exchange::server