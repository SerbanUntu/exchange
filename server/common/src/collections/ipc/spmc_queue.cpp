#include <server/common/collections/ipc/spmc_queue.hpp>

namespace exchange::server
{
template <TriviallyCopyable T> T SPMCQueue<T>::pop() noexcept
{
    T result = q.front();
    q.pop();
    return result;
}
template <TriviallyCopyable T> void SPMCQueue<T>::push(const T &item) noexcept
{
    q.push(item);
}
}; // namespace exchange::server