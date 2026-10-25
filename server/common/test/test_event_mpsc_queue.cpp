#include "server/common/collections/ipc/event_mpsc_queue.hpp"

#include "ipc_test_utils.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace exchange::server
{
namespace
{
using namespace std::chrono_literals;

template <typename Body> Event makeEvent(const OrderId orderId, const Body &body, const uint64_t accountId = 1)
{
    return Event{Event::Header{.orderId = orderId, .securityId = SecurityId{7}, .accountId = AccountId{accountId}},
                 body};
}

Event makeSequencedEvent(const uint64_t producer, const uint64_t sequence)
{
    return makeEvent(makeOrderId(1), NewMarketOrderEvent{.side = Side::BUY, .quantity = Quantity{sequence}}, producer);
}

void expectSameBody(const AmendLimitOrderEvent &expected, const AmendLimitOrderEvent &actual)
{
    EXPECT_EQ(actual.quantity, expected.quantity);
    EXPECT_EQ(actual.price, expected.price);
    EXPECT_EQ(actual.isQuantityAmended, expected.isQuantityAmended);
    EXPECT_EQ(actual.isPriceAmended, expected.isPriceAmended);
}

void expectSameBody(const CancelLimitOrderEvent &, const CancelLimitOrderEvent &)
{
}

void expectSameBody(const NewLimitOrderEvent &expected, const NewLimitOrderEvent &actual)
{
    EXPECT_EQ(actual.side, expected.side);
    EXPECT_EQ(actual.quantity, expected.quantity);
    EXPECT_EQ(actual.price, expected.price);
    EXPECT_EQ(actual.tif, expected.tif);
}

void expectSameBody(const NewMarketOrderEvent &expected, const NewMarketOrderEvent &actual)
{
    EXPECT_EQ(actual.side, expected.side);
    EXPECT_EQ(actual.quantity, expected.quantity);
}

void expectSameEvent(const Event &expected, const Event &actual)
{
    EXPECT_EQ(actual.header.orderId, expected.header.orderId);
    EXPECT_EQ(actual.header.securityId, expected.header.securityId);
    EXPECT_EQ(actual.header.accountId.value, expected.header.accountId.value);
    ASSERT_EQ(actual.body.index(), expected.body.index());
    std::visit(
        [&](const auto &expectedBody) {
            using Body = std::decay_t<decltype(expectedBody)>;
            expectSameBody(expectedBody, std::get<Body>(actual.body));
        },
        expected.body);
}
} // namespace

class EventMpscQueueTest : public SharedMemoryTest
{
};

TEST_F(EventMpscQueueTest, PopReturnsEveryEventTypeIntact)
{
    EventMpscQueue queue{sharedMemory};
    const std::array events{
        makeEvent(makeOrderId(1),
                  AmendLimitOrderEvent{
                      .quantity = Quantity{4}, .price = Price{99}, .isQuantityAmended = true, .isPriceAmended = false}),
        makeEvent(makeOrderId(2), CancelLimitOrderEvent{}),
        makeEvent(makeOrderId(3),
                  NewLimitOrderEvent{
                      .side = Side::SELL, .quantity = Quantity{10}, .price = Price{101}, .tif = TimeInForce::IOC}),
        makeEvent(makeOrderId(4), NewMarketOrderEvent{.side = Side::BUY, .quantity = Quantity{5}}),
    };

    for (const auto &event : events)
    {
        queue.push(event);
    }

    for (const auto &event : events)
    {
        expectSameEvent(event, queue.pop());
    }
}

TEST_F(EventMpscQueueTest, PopReturnsEventsInFifoOrder)
{
    EventMpscQueue queue{sharedMemory};

    for (uint64_t sequence = 0; sequence < 100; ++sequence)
    {
        queue.push(makeSequencedEvent(1, sequence));
    }

    for (uint64_t sequence = 0; sequence < 100; ++sequence)
    {
        expectSameEvent(makeSequencedEvent(1, sequence), queue.pop());
    }
}

TEST_F(EventMpscQueueTest, PopReturnsEventsPushedThroughSeparateHandles)
{
    EventMpscQueue consumer{sharedMemory};
    EventMpscQueue firstProducer{sharedMemory};
    EventMpscQueue secondProducer{sharedMemory};

    firstProducer.push(makeSequencedEvent(1, 0));
    secondProducer.push(makeSequencedEvent(2, 0));
    firstProducer.push(makeSequencedEvent(1, 1));

    expectSameEvent(makeSequencedEvent(1, 0), consumer.pop());
    expectSameEvent(makeSequencedEvent(2, 0), consumer.pop());
    expectSameEvent(makeSequencedEvent(1, 1), consumer.pop());
}

TEST_F(EventMpscQueueTest, ConcurrentProducersDeliverEveryEventInPerProducerOrder)
{
    constexpr std::size_t producerCount = 4;
    constexpr uint64_t eventsPerProducer = 200;

    EventMpscQueue consumer{sharedMemory};
    std::vector<EventMpscQueue> producers;
    for (std::size_t i = 0; i < producerCount; ++i)
    {
        producers.emplace_back(sharedMemory);
    }

    std::vector<std::jthread> threads;
    for (std::size_t producer = 0; producer < producerCount; ++producer)
    {
        threads.emplace_back([&queue = producers[producer], producer] {
            for (uint64_t sequence = 0; sequence < eventsPerProducer; ++sequence)
            {
                queue.push(makeSequencedEvent(producer, sequence));
            }
        });
    }

    std::array<uint64_t, producerCount> nextSequence{};
    for (std::size_t received = 0; received < producerCount * eventsPerProducer; ++received)
    {
        const auto event = consumer.pop();
        const auto producer = event.header.accountId.value;
        ASSERT_LT(producer, producerCount);
        EXPECT_EQ(std::get<NewMarketOrderEvent>(event.body).quantity, Quantity{nextSequence[producer]});
        ++nextSequence[producer];
    }

    for (const auto sequence : nextSequence)
    {
        EXPECT_EQ(sequence, eventsPerProducer);
    }
}

TEST_F(EventMpscQueueTest, PopBlocksUntilAnEventIsPushed)
{
    EventMpscQueue consumer{sharedMemory};
    EventMpscQueue producer{sharedMemory};

    auto popped = std::async(std::launch::async, [&consumer] { return consumer.pop(); });
    EXPECT_EQ(popped.wait_for(50ms), std::future_status::timeout);

    producer.push(makeSequencedEvent(1, 0));

    const bool isReady = popped.wait_for(5s) == std::future_status::ready;
    if (!isReady)
    {
        producer.push(makeSequencedEvent(1, 0));
    }
    ASSERT_TRUE(isReady);
    expectSameEvent(makeSequencedEvent(1, 0), popped.get());
}

TEST_F(EventMpscQueueTest, MovedQueueKeepsDeliveringPendingEvents)
{
    EventMpscQueue queue{sharedMemory};
    queue.push(makeSequencedEvent(1, 0));

    EventMpscQueue movedQueue{std::move(queue)};
    movedQueue.push(makeSequencedEvent(1, 1));

    expectSameEvent(makeSequencedEvent(1, 0), movedQueue.pop());
    expectSameEvent(makeSequencedEvent(1, 1), movedQueue.pop());
}

} // namespace exchange::server
