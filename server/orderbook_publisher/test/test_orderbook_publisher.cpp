#include "server/orderbook_publisher/orderbook_publisher.hpp"

#include "server/common/collections/ipc/action_spmc_receiver.hpp"
#include "server/common/collections/ipc/action_spmc_sender.hpp"
#include "server/common/collections/ipc/shared_memory.hpp"
#include "server/common/model/action/action.hpp"
#include "server/common/model/action/order_added_action.hpp"
#include "server/common/model/action/order_downsized_action.hpp"
#include "server/common/model/action/order_removed_action.hpp"
#include "server/common/model/action/order_upsized_action.hpp"
#include "server/common/model/action/trade_action.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace exchange::server
{
namespace
{
constexpr SecurityId SECURITY{1};
constexpr SecurityId OTHER_SECURITY{2};
using Levels = std::vector<std::pair<uint64_t, uint64_t>>;

OrderId makeOrderId(const uint8_t seed)
{
    boost::uuids::uuid uuid{};
    uuid.data[0] = seed;
    return OrderId{uuid};
}

template <typename Body> Action makeAction(const OrderId orderId, const Body &body, const SecurityId securityId)
{
    return Action{Action::Header{.orderId = orderId, .securityId = securityId, .timestamp = {}}, body};
}

Levels levelsOf(const std::array<L2Data::PriceQuantityPair, L2Data::MAX_LEVELS> &list, const uint8_t count)
{
    Levels levels;
    levels.reserve(count);
    for (std::size_t level{0}; level < count; ++level)
    {
        levels.emplace_back(list.at(level).price.value, list.at(level).quantity.value);
    }
    return levels;
}

} // namespace

class OrderbookPublisherTest : public testing::Test
{
  protected:
    SharedMemory sharedMemory{true, "TestOrderbookPublisherSharedMemory"};
    std::optional<OrderbookPublisher> publisher;

    OrderbookPublisherTest()
    {
        [[maybe_unused]] const auto _ =
            sharedMemory.get().construct<std::array<std::byte, SharedMemory::spmcTotalObjectSize>>(
                SharedMemory::spmcName)();
        publisher.emplace(ActionSpmcReceiver{sharedMemory});
    }

    void add(const OrderId orderId, const Side side, const uint64_t price, const uint64_t totalQuantity,
             const uint64_t filledQuantity = 0, const SecurityId securityId = SECURITY)
    {
        publisher->processAction(makeAction(orderId,
                                            OrderAddedAction{.price = Price{price},
                                                             .totalQuantity = Quantity{totalQuantity},
                                                             .filledQuantity = Quantity{filledQuantity},
                                                             .side = side},
                                            securityId));
    }

    void trade(const OrderId aggressorId, const OrderId restingId, const Side aggressorSide, const uint64_t price,
               const uint64_t quantity, const uint64_t remainingRestingQuantity)
    {
        publisher->processAction(makeAction(aggressorId,
                                            TradeAction{.restingId = restingId,
                                                        .price = Price{price},
                                                        .quantity = Quantity{quantity},
                                                        .remainingRestingQuantity = Quantity{remainingRestingQuantity},
                                                        .aggressorSide = aggressorSide},
                                            SECURITY));
        if (remainingRestingQuantity == 0)
        {
            remove(restingId);
        }
    }

    void remove(const OrderId orderId)
    {
        publisher->processAction(makeAction(orderId, OrderRemovedAction{}, SECURITY));
    }

    void downsize(const OrderId orderId, const uint64_t newQuantity)
    {
        publisher->processAction(
            makeAction(orderId, OrderDownsizedAction{.newQuantity = Quantity{newQuantity}}, SECURITY));
    }

    void upsize(const OrderId orderId, const uint64_t newQuantity)
    {
        publisher->processAction(
            makeAction(orderId, OrderUpsizedAction{.newQuantity = Quantity{newQuantity}}, SECURITY));
    }

    [[nodiscard]] Levels buyLevels(const uint8_t numberOfLevels = L2Data::MAX_LEVELS,
                                   const SecurityId securityId = SECURITY) const
    {
        const auto l2Data = publisher->getL2Data(securityId, numberOfLevels);
        return levelsOf(l2Data.buyList, l2Data.buyLevels);
    }

    [[nodiscard]] Levels sellLevels(const uint8_t numberOfLevels = L2Data::MAX_LEVELS,
                                    const SecurityId securityId = SECURITY) const
    {
        const auto l2Data = publisher->getL2Data(securityId, numberOfLevels);
        return levelsOf(l2Data.sellList, l2Data.sellLevels);
    }
};

TEST_F(OrderbookPublisherTest, GetL1DataReturnsAnEmptySnapshotForAnUnknownSecurity)
{
    const auto l1Data = publisher->getL1Data(SECURITY);

    EXPECT_EQ(l1Data.bestBuy, Price{0});
    EXPECT_EQ(l1Data.bestSell, Price{0});
    EXPECT_EQ(l1Data.bestBuyQuantity, Quantity{0});
    EXPECT_EQ(l1Data.bestSellQuantity, Quantity{0});
    EXPECT_EQ(l1Data.lastTradePrice, Price{0});
    EXPECT_EQ(l1Data.lastTradeQuantity, Quantity{0});
}

TEST_F(OrderbookPublisherTest, GetL2DataReturnsNoLevelsForAnUnknownSecurity)
{
    const auto l2Data = publisher->getL2Data(SECURITY, L2Data::MAX_LEVELS);

    EXPECT_EQ(l2Data.buyLevels, 0U);
    EXPECT_EQ(l2Data.sellLevels, 0U);
}

TEST_F(OrderbookPublisherTest, GetL1DataReportsTheBestPricesAndTheirAggregatedQuantities)
{
    add(makeOrderId(1), Side::BUY, 99, 5);
    add(makeOrderId(2), Side::BUY, 100, 3);
    add(makeOrderId(3), Side::BUY, 100, 4);
    add(makeOrderId(4), Side::SELL, 102, 6);
    add(makeOrderId(5), Side::SELL, 101, 2);

    const auto l1Data = publisher->getL1Data(SECURITY);

    EXPECT_EQ(l1Data.bestBuy, Price{100});
    EXPECT_EQ(l1Data.bestBuyQuantity, Quantity{7});
    EXPECT_EQ(l1Data.bestSell, Price{101});
    EXPECT_EQ(l1Data.bestSellQuantity, Quantity{2});
}

TEST_F(OrderbookPublisherTest, GetL1DataReportsAnEmptySideAsZero)
{
    add(makeOrderId(1), Side::BUY, 100, 5);

    const auto l1Data = publisher->getL1Data(SECURITY);

    EXPECT_EQ(l1Data.bestBuy, Price{100});
    EXPECT_EQ(l1Data.bestSell, Price{0});
    EXPECT_EQ(l1Data.bestSellQuantity, Quantity{0});
}

TEST_F(OrderbookPublisherTest, GetL1DataReportsTheMostRecentTrade)
{
    const auto resting = makeOrderId(1);
    add(resting, Side::SELL, 100, 10);

    trade(makeOrderId(2), resting, Side::BUY, 100, 4, 6);
    trade(makeOrderId(3), resting, Side::BUY, 100, 1, 5);

    const auto l1Data = publisher->getL1Data(SECURITY);
    EXPECT_EQ(l1Data.lastTradePrice, Price{100});
    EXPECT_EQ(l1Data.lastTradeQuantity, Quantity{1});
}

TEST_F(OrderbookPublisherTest, GetL2DataListsLevelsFromTheBestPriceOutwards)
{
    add(makeOrderId(1), Side::BUY, 98, 1);
    add(makeOrderId(2), Side::BUY, 100, 2);
    add(makeOrderId(3), Side::BUY, 99, 3);
    add(makeOrderId(4), Side::BUY, 100, 4);
    add(makeOrderId(5), Side::SELL, 103, 5);
    add(makeOrderId(6), Side::SELL, 101, 6);
    add(makeOrderId(7), Side::SELL, 102, 7);

    EXPECT_EQ(buyLevels(), (Levels{{100, 6}, {99, 3}, {98, 1}}));
    EXPECT_EQ(sellLevels(), (Levels{{101, 6}, {102, 7}, {103, 5}}));
}

TEST_F(OrderbookPublisherTest, GetL2DataStopsAtTheRequestedNumberOfLevels)
{
    for (uint8_t level{0}; level < 5; ++level)
    {
        add(makeOrderId(level), Side::BUY, 100 - level, 1);
        add(makeOrderId(level + 10), Side::SELL, 101 + level, 1);
    }

    EXPECT_EQ(buyLevels(2), (Levels{{100, 1}, {99, 1}}));
    EXPECT_EQ(sellLevels(2), (Levels{{101, 1}, {102, 1}}));
}

TEST_F(OrderbookPublisherTest, GetL2DataReportsEachSideUpToItsOwnDepth)
{
    add(makeOrderId(1), Side::BUY, 100, 1);
    add(makeOrderId(2), Side::SELL, 101, 1);
    add(makeOrderId(3), Side::SELL, 102, 1);
    add(makeOrderId(4), Side::SELL, 103, 1);

    const auto l2Data = publisher->getL2Data(SECURITY, 5);

    EXPECT_EQ(l2Data.buyLevels, 1U);
    EXPECT_EQ(l2Data.sellLevels, 3U);
}

TEST_F(OrderbookPublisherTest, GetL2DataCapsTheNumberOfLevelsAtTheMaximum)
{
    for (uint8_t level{0}; level < L2Data::MAX_LEVELS + 2; ++level)
    {
        add(makeOrderId(level), Side::BUY, 100 - level, 1);
    }

    const auto l2Data = publisher->getL2Data(SECURITY, UINT8_MAX);

    EXPECT_EQ(l2Data.buyLevels, L2Data::MAX_LEVELS);
    EXPECT_EQ(l2Data.buyList.back().price, Price{91});
}

TEST_F(OrderbookPublisherTest, ProcessActionAddsOnlyTheUnfilledPartOfAPartiallyFilledOrder)
{
    add(makeOrderId(1), Side::BUY, 100, 10, 3);

    EXPECT_EQ(buyLevels(), (Levels{{100, 7}}));
}

TEST_F(OrderbookPublisherTest, ProcessActionDownsizesAnOrderThatWasPartiallyFilledWhenAdded)
{
    const auto orderId = makeOrderId(1);
    add(orderId, Side::BUY, 100, 10, 3);

    downsize(orderId, 8);

    EXPECT_EQ(buyLevels(), (Levels{{100, 5}}));
}

TEST_F(OrderbookPublisherTest, ProcessActionReducesARestingOrderByAPartialFill)
{
    const auto resting = makeOrderId(1);
    add(resting, Side::SELL, 100, 10);

    trade(makeOrderId(2), resting, Side::BUY, 100, 4, 6);

    EXPECT_EQ(sellLevels(), (Levels{{100, 6}}));
}

TEST_F(OrderbookPublisherTest, ProcessActionAppliesSuccessivePartialFills)
{
    const auto resting = makeOrderId(1);
    add(resting, Side::SELL, 100, 10);

    trade(makeOrderId(2), resting, Side::BUY, 100, 4, 6);
    trade(makeOrderId(3), resting, Side::BUY, 100, 2, 4);
    trade(makeOrderId(4), resting, Side::BUY, 100, 1, 3);

    EXPECT_EQ(sellLevels(), (Levels{{100, 3}}));
}

TEST_F(OrderbookPublisherTest, ProcessActionDownsizesAPartiallyFilledOrderRelativeToItsTotalQuantity)
{
    const auto resting = makeOrderId(1);
    add(resting, Side::SELL, 100, 10);
    trade(makeOrderId(2), resting, Side::BUY, 100, 4, 6);

    downsize(resting, 8);

    EXPECT_EQ(sellLevels(), (Levels{{100, 4}}));
}

TEST_F(OrderbookPublisherTest, ProcessActionUpsizesAPartiallyFilledOrderRelativeToItsTotalQuantity)
{
    const auto resting = makeOrderId(1);
    add(resting, Side::SELL, 100, 10);
    trade(makeOrderId(2), resting, Side::BUY, 100, 4, 6);

    upsize(resting, 12);

    EXPECT_EQ(sellLevels(), (Levels{{100, 8}}));
}

TEST_F(OrderbookPublisherTest, ProcessActionOnlyFillsTheTradedOrderWithinAPriceLevel)
{
    const auto first = makeOrderId(1);
    const auto second = makeOrderId(2);
    add(first, Side::BUY, 100, 5);
    add(second, Side::BUY, 100, 5);
    trade(makeOrderId(3), first, Side::SELL, 100, 2, 3);

    downsize(first, 4);
    trade(makeOrderId(4), second, Side::SELL, 100, 1, 4);

    EXPECT_EQ(buyLevels(), (Levels{{100, 6}}));
}

TEST_F(OrderbookPublisherTest, ProcessActionRemovesAFullyFilledOrder)
{
    const auto resting = makeOrderId(1);
    add(resting, Side::SELL, 100, 5);
    add(makeOrderId(2), Side::SELL, 101, 5);

    trade(makeOrderId(3), resting, Side::BUY, 100, 5, 0);

    EXPECT_EQ(sellLevels(), (Levels{{101, 5}}));
}

TEST_F(OrderbookPublisherTest, ProcessActionRemovesACancelledOrder)
{
    const auto cancelled = makeOrderId(1);
    add(cancelled, Side::BUY, 100, 5);
    add(makeOrderId(2), Side::BUY, 99, 5);

    remove(cancelled);

    EXPECT_EQ(buyLevels(), (Levels{{99, 5}}));
}

TEST_F(OrderbookPublisherTest, ProcessActionRepricesAPartiallyFilledOrder)
{
    const auto resting = makeOrderId(1);
    add(resting, Side::SELL, 100, 10);
    trade(makeOrderId(2), resting, Side::BUY, 100, 4, 6);

    remove(resting);
    add(resting, Side::SELL, 99, 10, 4);
    downsize(resting, 8);

    EXPECT_EQ(sellLevels(), (Levels{{99, 4}}));
}

TEST_F(OrderbookPublisherTest, ProcessActionAddsTheRemainderOfAnAggressorAfterItsTrades)
{
    const auto resting = makeOrderId(1);
    const auto aggressor = makeOrderId(2);
    add(resting, Side::SELL, 100, 3);

    trade(aggressor, resting, Side::BUY, 100, 3, 0);
    add(aggressor, Side::BUY, 100, 10, 3);

    EXPECT_EQ(buyLevels(), (Levels{{100, 7}}));
    EXPECT_TRUE(sellLevels().empty());
}

TEST_F(OrderbookPublisherTest, ProcessActionKeepsAnOrderBookPerSecurity)
{
    add(makeOrderId(1), Side::BUY, 100, 5, 0, SECURITY);
    add(makeOrderId(2), Side::BUY, 200, 7, 0, OTHER_SECURITY);

    EXPECT_EQ(buyLevels(L2Data::MAX_LEVELS, SECURITY), (Levels{{100, 5}}));
    EXPECT_EQ(buyLevels(L2Data::MAX_LEVELS, OTHER_SECURITY), (Levels{{200, 7}}));
}

TEST_F(OrderbookPublisherTest, LoopProcessesActionsFromTheQueue)
{
    const auto resting = makeOrderId(1);
    ActionSpmcSender sender{sharedMemory};
    sender.send(makeAction(
        resting,
        OrderAddedAction{
            .price = Price{100}, .totalQuantity = Quantity{10}, .filledQuantity = Quantity{0}, .side = Side::SELL},
        SECURITY));
    sender.send(makeAction(makeOrderId(2),
                           TradeAction{.restingId = resting,
                                       .price = Price{100},
                                       .quantity = Quantity{4},
                                       .remainingRestingQuantity = Quantity{6},
                                       .aggressorSide = Side::BUY},
                           SECURITY));

    publisher->loop();
    publisher->loop();

    const auto l1Data = publisher->getL1Data(SECURITY);
    EXPECT_EQ(l1Data.bestSell, Price{100});
    EXPECT_EQ(l1Data.bestSellQuantity, Quantity{6});
    EXPECT_EQ(l1Data.lastTradeQuantity, Quantity{4});
}

} // namespace exchange::server
