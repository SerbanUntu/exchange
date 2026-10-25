#pragma once
#include "shared_memory.hpp"
#include "server/common/model/action/action.hpp"
#include <type_traits>

extern "C"
{
#include "concurrent/aeron_broadcast_transmitter.h"
}

namespace exchange::server
{

class ActionSpmcSender
{
    static constexpr int32_t messageTypeId{1};

    aeron_broadcast_transmitter_t transmitter;

  public:
    explicit ActionSpmcSender(SharedMemory &sharedMemory);

    /**
     * Broadcast an action immediately to the SPMC queue.
     *
     * @param action The action to broadcast via the SPMC queue.
     */
    void send(const Action &action) noexcept;
};

} // namespace exchange::server