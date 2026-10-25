#include "server/common/collections/ipc/action_spmc_sender.hpp"
#include <array>

namespace exchange::server
{

ActionSpmcSender::ActionSpmcSender(SharedMemory &sharedMemory)
{
    auto [region, _] =
        sharedMemory.get().find<std::array<std::byte, SharedMemory::spmcTotalObjectSize>>(SharedMemory::spmcName);
    aeron_broadcast_transmitter_init(&transmitter, region, SharedMemory::spmcTotalObjectSize);
}

void ActionSpmcSender::send(const Action &action) noexcept
{
    aeron_broadcast_transmitter_transmit(&transmitter, messageTypeId, &action, sizeof action);
}

} // namespace exchange::server