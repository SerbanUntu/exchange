#pragma once
#include "shared_memory.hpp"
#include "server/common/model/action/action.hpp"

#include <type_traits>
extern "C"
{
#include "concurrent/aeron_broadcast_receiver.h"
}

namespace exchange::server
{

class ActionSpmcReceiver
{
    aeron_broadcast_receiver_t receiver;

    static void onAction(std::int32_t, uint8_t *data, std::size_t, void *clientd) noexcept;

  public:
    ActionSpmcReceiver(SharedMemory &shared_memory);
    ActionSpmcReceiver(const ActionSpmcReceiver &) = delete;
    ActionSpmcReceiver &operator=(const ActionSpmcReceiver &) = delete;
    ActionSpmcReceiver(ActionSpmcReceiver &&other) noexcept;
    ActionSpmcReceiver &operator=(ActionSpmcReceiver &&other) noexcept;
    ~ActionSpmcReceiver();

    /**
     * Busy wait until another broadcasted action is available from the SPMC queue.
     *
     * @return An action received from the SPMC queue.
     */
    [[nodiscard]] Action receive() noexcept;
};

} // namespace exchange::server
