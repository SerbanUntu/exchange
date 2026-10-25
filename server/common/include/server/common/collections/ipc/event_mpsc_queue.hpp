#pragma once
#include "shared_memory.hpp"
#include "server/common/model/event/event.hpp"
#include <type_traits>

extern "C"
{
#include "concurrent/aeron_mpsc_rb.h"
}

namespace exchange::server
{

class EventMpscQueue
{
    static constexpr int32_t messageTypeId{1};

    aeron_mpsc_rb_t ringBufferWrapper{};

    static void onEvent(std::int32_t msgTypeId, const void *data, std::size_t length, void *clientd) noexcept;

  public:
    explicit EventMpscQueue(SharedMemory &sharedMemory);

    /**
     * Busy wait until there is an event to consume in the MPSC queue.
     *
     * @return The event received the MPSC queue.
     */
    [[nodiscard]] Event pop() noexcept; // blocking

    /**
     * Push an event immediately to the MPSC queue.
     *
     * @param event The event to push to the MPSC queue.
     */
    void push(const Event &event) noexcept;
};

} // namespace exchange::server