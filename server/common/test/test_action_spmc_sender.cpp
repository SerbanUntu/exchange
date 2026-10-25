#include "server/common/collections/ipc/action_spmc_receiver.hpp"
#include "server/common/collections/ipc/action_spmc_sender.hpp"

#include "ipc_test_utils.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <utility>

namespace exchange::server
{

class ActionSpmcSenderTest : public SharedMemoryTest
{
};

TEST_F(ActionSpmcSenderTest, SendDeliversEveryActionTypeIntact)
{
    ActionSpmcReceiver receiver{sharedMemory};
    ActionSpmcSender sender{sharedMemory};
    const std::array actions{
        makeAction(makeOrderId(1), OrderAddedAction{.price = Price{100},
                                                    .totalQuantity = Quantity{10},
                                                    .filledQuantity = Quantity{3},
                                                    .side = Side::BUY}),
        makeAction(makeOrderId(2), OrderDownsizedAction{.newQuantity = Quantity{4}}),
        makeAction(makeOrderId(3), OrderRemovedAction{}),
        makeAction(makeOrderId(4), OrderUpsizedAction{.newQuantity = Quantity{15}}),
        makeAction(makeOrderId(5), TradeAction{.restingId = makeOrderId(6),
                                               .price = Price{101},
                                               .quantity = Quantity{2},
                                               .aggressorSide = Side::SELL}),
    };

    for (const auto &action : actions)
    {
        sender.send(action);
    }

    for (const auto &action : actions)
    {
        expectSameAction(action, receiver.receive());
    }
}

TEST_F(ActionSpmcSenderTest, SendDeliversActionsInOrder)
{
    ActionSpmcReceiver receiver{sharedMemory};
    ActionSpmcSender sender{sharedMemory};

    for (uint64_t quantity = 1; quantity <= 100; ++quantity)
    {
        sender.send(makeTrade(quantity));
    }

    for (uint64_t quantity = 1; quantity <= 100; ++quantity)
    {
        expectSameAction(makeTrade(quantity), receiver.receive());
    }
}

TEST_F(ActionSpmcSenderTest, SendInterleavedWithReceiveDeliversEachAction)
{
    ActionSpmcReceiver receiver{sharedMemory};
    ActionSpmcSender sender{sharedMemory};

    for (uint64_t quantity = 1; quantity <= 10; ++quantity)
    {
        sender.send(makeTrade(quantity));
        expectSameAction(makeTrade(quantity), receiver.receive());
    }
}

TEST_F(ActionSpmcSenderTest, MovedSenderKeepsDelivering)
{
    ActionSpmcReceiver receiver{sharedMemory};
    ActionSpmcSender sender{sharedMemory};
    sender.send(makeTrade(1));

    ActionSpmcSender movedSender{std::move(sender)};
    movedSender.send(makeTrade(2));

    expectSameAction(makeTrade(1), receiver.receive());
    expectSameAction(makeTrade(2), receiver.receive());
}

} // namespace exchange::server
