#include "server/common/collections/ipc/action_spmc_receiver.hpp"
#include <array>
#include <cstring>
#include <utility>

namespace exchange::server
{

ActionSpmcReceiver::ActionSpmcReceiver(SharedMemory &sharedMemory)
{
    auto [region, _] =
        sharedMemory.get().find<std::array<std::byte, SharedMemory::spmcTotalObjectSize>>(SharedMemory::spmcName);
    aeron_broadcast_receiver_init(&receiver, region, SharedMemory::spmcTotalObjectSize);
}

ActionSpmcReceiver::ActionSpmcReceiver(ActionSpmcReceiver &&other) noexcept : receiver{other.receiver}
{
    other.receiver.scratch_buffer = nullptr;
    other.receiver.scratch_buffer_capacity = 0;
}

ActionSpmcReceiver &ActionSpmcReceiver::operator=(ActionSpmcReceiver &&other) noexcept
{
    std::swap(receiver, other.receiver);
    return *this;
}

ActionSpmcReceiver::~ActionSpmcReceiver()
{
    aeron_broadcast_receiver_close(&receiver);
}

void ActionSpmcReceiver::onAction([[maybe_unused]] std::int32_t msgTypeId, uint8_t *data,
                                  [[maybe_unused]] std::size_t length, void *clientd) noexcept
{
    std::memcpy(clientd, data, sizeof(Action));
}

Action ActionSpmcReceiver::receive() noexcept
{
    Action action;
    while (aeron_broadcast_receiver_receive(&receiver, &onAction, &action) != 1)
    {
    }
    return action;
}

} // namespace exchange::server