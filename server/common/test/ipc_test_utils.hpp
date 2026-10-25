#pragma once
#include "server/common/collections/ipc/shared_memory.hpp"
#include "server/common/model/action/action.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <variant>

namespace exchange::server
{

class SharedMemoryTest : public testing::Test
{
  protected:
    SharedMemory sharedMemory{true, "TestIpcSharedMemory"};

    SharedMemoryTest()
    {
        sharedMemory.get().construct<std::array<std::byte, SharedMemory::mpscTotalObjectSize>>(
            SharedMemory::mpscName)();
        sharedMemory.get().construct<std::array<std::byte, SharedMemory::spmcTotalObjectSize>>(
            SharedMemory::spmcName)();
    }
};

inline OrderId makeOrderId(const uint8_t seed)
{
    boost::uuids::uuid uuid{};
    uuid.data[0] = seed;
    return OrderId{uuid};
}

template <typename Body> Action makeAction(const OrderId orderId, const Body &body)
{
    return Action{Action::Header{.orderId = orderId,
                                 .securityId = SecurityId{7},
                                 .timestamp = TimestampNs{std::chrono::nanoseconds{1'700'000'000'123'456'789}}},
                  body};
}

inline Action makeTrade(const uint64_t quantity)
{
    return makeAction(makeOrderId(1), TradeAction{.restingId = makeOrderId(2),
                                                  .price = Price{100},
                                                  .quantity = Quantity{quantity},
                                                  .aggressorSide = Side::BUY});
}

inline void expectSameBody(const OrderAddedAction &expected, const OrderAddedAction &actual)
{
    EXPECT_EQ(actual.price, expected.price);
    EXPECT_EQ(actual.totalQuantity, expected.totalQuantity);
    EXPECT_EQ(actual.filledQuantity, expected.filledQuantity);
    EXPECT_EQ(actual.side, expected.side);
}

inline void expectSameBody(const OrderDownsizedAction &expected, const OrderDownsizedAction &actual)
{
    EXPECT_EQ(actual.newQuantity, expected.newQuantity);
}

inline void expectSameBody(const OrderRemovedAction &, const OrderRemovedAction &)
{
}

inline void expectSameBody(const OrderUpsizedAction &expected, const OrderUpsizedAction &actual)
{
    EXPECT_EQ(actual.newQuantity, expected.newQuantity);
}

inline void expectSameBody(const TradeAction &expected, const TradeAction &actual)
{
    EXPECT_EQ(actual.restingId, expected.restingId);
    EXPECT_EQ(actual.price, expected.price);
    EXPECT_EQ(actual.quantity, expected.quantity);
    EXPECT_EQ(actual.aggressorSide, expected.aggressorSide);
}

inline void expectSameAction(const Action &expected, const Action &actual)
{
    EXPECT_EQ(actual.header.orderId, expected.header.orderId);
    EXPECT_EQ(actual.header.securityId, expected.header.securityId);
    EXPECT_EQ(actual.header.timestamp.time_since_epoch().count(), expected.header.timestamp.time_since_epoch().count());
    ASSERT_EQ(actual.body.index(), expected.body.index());
    std::visit(
        [&](const auto &expectedBody) {
            using Body = std::decay_t<decltype(expectedBody)>;
            expectSameBody(expectedBody, std::get<Body>(actual.body));
        },
        expected.body);
}

} // namespace exchange::server
