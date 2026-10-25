#include "server/common/collections/ipc/event_mpsc_queue.hpp"
#include <array>
#include <cstring>

namespace exchange::server
{

EventMpscQueue::EventMpscQueue(SharedMemory &sharedMemory)
{
    auto [region, _] =
        sharedMemory.get().find<std::array<std::byte, SharedMemory::mpscTotalObjectSize>>(SharedMemory::mpscName);
    aeron_mpsc_rb_init(&ringBufferWrapper, region, SharedMemory::mpscTotalObjectSize);
}

void EventMpscQueue::push(const Event &event) noexcept
{
    aeron_mpsc_rb_write(&ringBufferWrapper, messageTypeId, &event, sizeof event);
}

void EventMpscQueue::onEvent(std::int32_t, const void *data, std::size_t, void *clientd) noexcept
{
    std::memcpy(clientd, data, sizeof(Event));
}

Event EventMpscQueue::pop() noexcept
{
    Event event;
    while (aeron_mpsc_rb_read(&ringBufferWrapper, &onEvent, &event, 1) != 1)
    {
    }
    return event;
}

} // namespace exchange::server