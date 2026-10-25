#include "server/common/collections/ipc/action_spmc_receiver.hpp"
#include "server/common/collections/ipc/action_spmc_sender.hpp"

#include "ipc_test_utils.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <future>
#include <utility>

namespace exchange::server
{
using namespace std::chrono_literals;

class ActionSpmcReceiverTest : public SharedMemoryTest
{
};

TEST_F(ActionSpmcReceiverTest, EveryReceiverGetsEveryAction)
{
    ActionSpmcReceiver first{sharedMemory};
    ActionSpmcReceiver second{sharedMemory};
    ActionSpmcSender sender{sharedMemory};

    for (uint64_t quantity = 1; quantity <= 5; ++quantity)
    {
        sender.send(makeTrade(quantity));
    }

    for (uint64_t quantity = 1; quantity <= 5; ++quantity)
    {
        expectSameAction(makeTrade(quantity), first.receive());
    }
    for (uint64_t quantity = 1; quantity <= 5; ++quantity)
    {
        expectSameAction(makeTrade(quantity), second.receive());
    }
}

TEST_F(ActionSpmcReceiverTest, ReceiversProgressIndependently)
{
    ActionSpmcReceiver fast{sharedMemory};
    ActionSpmcReceiver slow{sharedMemory};
    ActionSpmcSender sender{sharedMemory};

    sender.send(makeTrade(1));
    expectSameAction(makeTrade(1), fast.receive());
    sender.send(makeTrade(2));
    expectSameAction(makeTrade(2), fast.receive());

    expectSameAction(makeTrade(1), slow.receive());
    expectSameAction(makeTrade(2), slow.receive());
}

TEST_F(ActionSpmcReceiverTest, ReceiveBlocksUntilAnActionIsSent)
{
    ActionSpmcReceiver receiver{sharedMemory};
    ActionSpmcSender sender{sharedMemory};

    auto received = std::async(std::launch::async, [&receiver] { return receiver.receive(); });
    EXPECT_EQ(received.wait_for(50ms), std::future_status::timeout);

    sender.send(makeTrade(1));

    const bool isReady = received.wait_for(5s) == std::future_status::ready;
    if (!isReady)
    {
        sender.send(makeTrade(1));
    }
    ASSERT_TRUE(isReady);
    expectSameAction(makeTrade(1), received.get());
}

TEST_F(ActionSpmcReceiverTest, MovedReceiverContinuesWhereTheOriginalLeftOff)
{
    ActionSpmcReceiver receiver{sharedMemory};
    ActionSpmcSender sender{sharedMemory};
    sender.send(makeTrade(1));
    sender.send(makeTrade(2));
    expectSameAction(makeTrade(1), receiver.receive());

    ActionSpmcReceiver movedReceiver{std::move(receiver)};
    sender.send(makeTrade(3));

    expectSameAction(makeTrade(2), movedReceiver.receive());
    expectSameAction(makeTrade(3), movedReceiver.receive());
}

TEST_F(ActionSpmcReceiverTest, MoveAssignedReceiverContinuesWhereTheSourceLeftOff)
{
    ActionSpmcReceiver source{sharedMemory};
    ActionSpmcReceiver target{sharedMemory};
    ActionSpmcSender sender{sharedMemory};
    sender.send(makeTrade(1));
    sender.send(makeTrade(2));
    expectSameAction(makeTrade(1), source.receive());

    target = std::move(source);
    sender.send(makeTrade(3));

    expectSameAction(makeTrade(2), target.receive());
    expectSameAction(makeTrade(3), target.receive());
}

} // namespace exchange::server
