#include "server/matching_engine/process_event.hpp"

#include "server/common/model/action/order_added_action.hpp"
#include "server/common/model/action/order_downsized_action.hpp"
#include "server/common/model/action/order_upsized_action.hpp"
#include "server/common/model/action/trade_action.hpp"
#include "server/common/model/event/amend_limit_order_event.hpp"
#include "server/common/model/event/cancel_limit_order_event.hpp"
#include "server/common/model/event/new_limit_order_event.hpp"
#include "server/common/model/event/new_market_order_event.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace exchange::server
{
namespace
{
constexpr uint32_t SECURITY = 1;
constexpr uint32_t OTHER_SECURITY = 2;

OrderId makeOrderId(const uint8_t seed, const uint32_t security = SECURITY)
{
    boost::uuids::uuid uuid{};
    uuid.data[0] = seed;
    return OrderId{uuid, SecurityId{security}};
}

std::unordered_map<SecurityId, OrderBook> makeState()
{
    std::unordered_map<SecurityId, OrderBook> state;
    state.try_emplace(SecurityId{SECURITY});
    state.try_emplace(SecurityId{OTHER_SECURITY});
    return state;
}

std::vector<std::unique_ptr<Action>> restGtc(std::unordered_map<SecurityId, OrderBook> &state, const OrderId orderId,
                                             const Side side, const uint64_t price, const uint64_t quantity)
{
    const NewLimitOrderEvent event{orderId, side, Quantity{quantity}, Price{price}, TimeInForce::GTC};
    return processEvent(event, state);
}

std::vector<ActionType> typesOf(const std::vector<std::unique_ptr<Action>> &actions)
{
    std::vector<ActionType> types;
    types.reserve(actions.size());
    for (const auto &action : actions)
    {
        types.push_back(action->getType());
    }
    return types;
}
} // namespace

TEST(ProcessEventTest, ProcessEventDoesNotKeepNonGTCOrdersOnTheBook)
{
    auto state = makeState();
    const auto aggressor = makeOrderId(2);
    restGtc(state, makeOrderId(1), Side::SELL, 100, 2);

    const NewLimitOrderEvent iocEvent{aggressor, Side::BUY, Quantity{5}, Price{100}, TimeInForce::IOC};
    const auto actions = processEvent(iocEvent, state);

    EXPECT_EQ(typesOf(actions), (std::vector{ActionType::TRADE, ActionType::ORDER_REMOVED}));

    const CancelLimitOrderEvent cancelEvent{aggressor};
    EXPECT_TRUE(processEvent(cancelEvent, state).empty());
    EXPECT_EQ(typesOf(restGtc(state, makeOrderId(3), Side::SELL, 100, 3)), (std::vector{ActionType::ORDER_ADDED}));
}

TEST(ProcessEventTest, ProcessEventAddsAGtcLimitOrderToTheBook)
{
    auto state = makeState();
    const auto orderId = makeOrderId(1);

    const auto actions = restGtc(state, orderId, Side::BUY, 100, 5);

    ASSERT_EQ(actions.size(), 1U);
    const auto &added = dynamic_cast<const OrderAddedAction &>(*actions[0]);
    EXPECT_EQ(added.orderId, orderId);
    EXPECT_EQ(added.price, Price{100});
    EXPECT_EQ(added.totalQuantity, Quantity{5});
    EXPECT_EQ(added.filledQuantity, Quantity{0});
    EXPECT_EQ(added.side, Side::BUY);
}

TEST(ProcessEventTest, ProcessEventEmitsATradeAndRemovesTheFilledRestingOrder)
{
    auto state = makeState();
    const auto resting = makeOrderId(1);
    const auto aggressor = makeOrderId(2);
    restGtc(state, resting, Side::SELL, 100, 5);

    const auto actions = restGtc(state, aggressor, Side::BUY, 100, 5);

    ASSERT_EQ(typesOf(actions), (std::vector{ActionType::TRADE, ActionType::ORDER_REMOVED}));
    const auto &trade = dynamic_cast<const TradeAction &>(*actions[0]);
    EXPECT_EQ(trade.orderId, aggressor);
    EXPECT_EQ(trade.restingId, resting);
    EXPECT_EQ(trade.price, Price{100});
    EXPECT_EQ(trade.quantity, Quantity{5});
    EXPECT_EQ(trade.aggressorSide, Side::BUY);
    EXPECT_EQ(actions[1]->orderId, resting);
}

TEST(ProcessEventTest, ProcessEventDoesNotRemoveAPartiallyFilledRestingOrder)
{
    auto state = makeState();
    const auto resting = makeOrderId(1);
    restGtc(state, resting, Side::SELL, 100, 10);

    const NewLimitOrderEvent event{makeOrderId(2), Side::BUY, Quantity{4}, Price{100}, TimeInForce::IOC};
    const auto actions = processEvent(event, state);

    EXPECT_EQ(typesOf(actions), (std::vector{ActionType::TRADE}));
}

TEST(ProcessEventTest, ProcessEventAddsTheUnfilledRemainderOfAGtcOrder)
{
    auto state = makeState();
    const auto aggressor = makeOrderId(2);
    restGtc(state, makeOrderId(1), Side::SELL, 100, 3);

    const auto actions = restGtc(state, aggressor, Side::BUY, 100, 10);

    ASSERT_EQ(typesOf(actions), (std::vector{ActionType::TRADE, ActionType::ORDER_REMOVED, ActionType::ORDER_ADDED}));
    const auto &added = dynamic_cast<const OrderAddedAction &>(*actions[2]);
    EXPECT_EQ(added.orderId, aggressor);
    EXPECT_EQ(added.totalQuantity, Quantity{10});
    EXPECT_EQ(added.filledQuantity, Quantity{3});
}

TEST(ProcessEventTest, ProcessEventEmitsTradesForAMarketOrderWithoutAddingIt)
{
    auto state = makeState();
    const auto resting = makeOrderId(1);
    const auto aggressor = makeOrderId(2);
    restGtc(state, resting, Side::SELL, 100, 5);

    const NewMarketOrderEvent event{aggressor, Side::BUY, Quantity{5}};
    const auto actions = processEvent(event, state);

    ASSERT_EQ(typesOf(actions), (std::vector{ActionType::TRADE, ActionType::ORDER_REMOVED}));
    EXPECT_EQ(dynamic_cast<const TradeAction &>(*actions[0]).price, Price{100});
    EXPECT_EQ(actions[1]->orderId, resting);
}

TEST(ProcessEventTest, ProcessEventEmitsNoActionsForAMarketOrderWithoutLiquidity)
{
    auto state = makeState();
    const NewMarketOrderEvent event{makeOrderId(1), Side::BUY, Quantity{5}};

    EXPECT_TRUE(processEvent(event, state).empty());
}

TEST(ProcessEventTest, ProcessEventEmitsNoActionsForAnUnfillableFokOrder)
{
    auto state = makeState();
    restGtc(state, makeOrderId(1), Side::SELL, 100, 2);

    const NewLimitOrderEvent event{makeOrderId(2), Side::BUY, Quantity{5}, Price{100}, TimeInForce::FOK};

    EXPECT_TRUE(processEvent(event, state).empty());
}

TEST(ProcessEventTest, ProcessEventDownsizesAnOrder)
{
    auto state = makeState();
    const auto orderId = makeOrderId(1);
    restGtc(state, orderId, Side::SELL, 100, 10);

    const AmendLimitOrderEvent event{orderId, Quantity{4}, std::nullopt};
    const auto actions = processEvent(event, state);

    ASSERT_EQ(typesOf(actions), (std::vector{ActionType::ORDER_DOWNSIZED}));
    const auto &downsized = dynamic_cast<const OrderDownsizedAction &>(*actions[0]);
    EXPECT_EQ(downsized.orderId, orderId);
    EXPECT_EQ(downsized.newQuantity, Quantity{4});
}

TEST(ProcessEventTest, ProcessEventUpsizesAnOrder)
{
    auto state = makeState();
    const auto orderId = makeOrderId(1);
    restGtc(state, orderId, Side::SELL, 100, 10);

    const AmendLimitOrderEvent event{orderId, Quantity{15}, std::nullopt};
    const auto actions = processEvent(event, state);

    ASSERT_EQ(typesOf(actions), (std::vector{ActionType::ORDER_UPSIZED}));
    const auto &upsized = dynamic_cast<const OrderUpsizedAction &>(*actions[0]);
    EXPECT_EQ(upsized.orderId, orderId);
    EXPECT_EQ(upsized.newQuantity, Quantity{15});
}

TEST(ProcessEventTest, ProcessEventRemovesAnOrderAmendedDownToItsFilledQuantity)
{
    auto state = makeState();
    const auto resting = makeOrderId(1);
    restGtc(state, resting, Side::SELL, 100, 10);
    restGtc(state, makeOrderId(2), Side::BUY, 100, 4);

    const AmendLimitOrderEvent event{resting, Quantity{4}, std::nullopt};
    const auto actions = processEvent(event, state);

    ASSERT_EQ(typesOf(actions), (std::vector{ActionType::ORDER_REMOVED}));
    EXPECT_EQ(actions[0]->orderId, resting);
}

TEST(ProcessEventTest, ProcessEventEmitsNoActionsForARejectedAmend)
{
    auto state = makeState();
    const AmendLimitOrderEvent unknownOrder{makeOrderId(1), Quantity{5}, std::nullopt};
    EXPECT_TRUE(processEvent(unknownOrder, state).empty());

    const auto orderId = makeOrderId(2);
    restGtc(state, orderId, Side::SELL, 100, 10);
    const AmendLimitOrderEvent unchangedQuantity{orderId, Quantity{10}, std::nullopt};
    EXPECT_TRUE(processEvent(unchangedQuantity, state).empty());
}

TEST(ProcessEventTest, ProcessEventRepricesAnOrderAsARemovalAndAnAddition)
{
    auto state = makeState();
    const auto orderId = makeOrderId(1);
    restGtc(state, orderId, Side::BUY, 100, 5);

    const AmendLimitOrderEvent event{orderId, std::nullopt, Price{99}};
    const auto actions = processEvent(event, state);

    ASSERT_EQ(typesOf(actions), (std::vector{ActionType::ORDER_REMOVED, ActionType::ORDER_ADDED}));
    EXPECT_EQ(actions[0]->orderId, orderId);
    const auto &added = dynamic_cast<const OrderAddedAction &>(*actions[1]);
    EXPECT_EQ(added.orderId, orderId);
    EXPECT_EQ(added.price, Price{99});
    EXPECT_EQ(added.totalQuantity, Quantity{5});
}

TEST(ProcessEventTest, ProcessEventEmitsTradesWhenAnAmendedPriceCrosses)
{
    auto state = makeState();
    const auto counterparty = makeOrderId(1);
    const auto orderId = makeOrderId(2);
    restGtc(state, counterparty, Side::SELL, 105, 5);
    restGtc(state, orderId, Side::BUY, 100, 5);

    const AmendLimitOrderEvent event{orderId, std::nullopt, Price{105}};
    const auto actions = processEvent(event, state);

    ASSERT_EQ(typesOf(actions), (std::vector{ActionType::ORDER_REMOVED, ActionType::TRADE, ActionType::ORDER_REMOVED}));
    EXPECT_EQ(actions[0]->orderId, orderId);
    const auto &trade = dynamic_cast<const TradeAction &>(*actions[1]);
    EXPECT_EQ(trade.orderId, orderId);
    EXPECT_EQ(trade.restingId, counterparty);
    EXPECT_EQ(trade.price, Price{105});
    EXPECT_EQ(actions[2]->orderId, counterparty);
}

TEST(ProcessEventTest, ProcessEventDownsizesAnOrderWhenTheAmendRestatesTheCurrentPrice)
{
    auto state = makeState();
    const auto orderId = makeOrderId(1);
    restGtc(state, orderId, Side::SELL, 100, 10);

    const AmendLimitOrderEvent event{orderId, Quantity{4}, Price{100}};
    const auto actions = processEvent(event, state);

    ASSERT_EQ(typesOf(actions), (std::vector{ActionType::ORDER_DOWNSIZED}));
    EXPECT_EQ(dynamic_cast<const OrderDownsizedAction &>(*actions[0]).newQuantity, Quantity{4});
}

TEST(ProcessEventTest, ProcessEventCancelsARestingOrder)
{
    auto state = makeState();
    const auto orderId = makeOrderId(1);
    restGtc(state, orderId, Side::SELL, 100, 5);

    const CancelLimitOrderEvent event{orderId};
    const auto actions = processEvent(event, state);

    ASSERT_EQ(typesOf(actions), (std::vector{ActionType::ORDER_REMOVED}));
    EXPECT_EQ(actions[0]->orderId, orderId);
    EXPECT_EQ(typesOf(restGtc(state, makeOrderId(2), Side::BUY, 100, 5)), (std::vector{ActionType::ORDER_ADDED}));
}

TEST(ProcessEventTest, ProcessEventEmitsNoActionsWhenCancellingAnUnknownOrder)
{
    auto state = makeState();
    const CancelLimitOrderEvent event{makeOrderId(1)};

    EXPECT_TRUE(processEvent(event, state).empty());
}

TEST(ProcessEventTest, ProcessEventKeepsAnOrderBookPerSecurity)
{
    auto state = makeState();
    restGtc(state, makeOrderId(1, SECURITY), Side::SELL, 100, 5);

    const auto actions = restGtc(state, makeOrderId(2, OTHER_SECURITY), Side::BUY, 100, 5);

    EXPECT_EQ(typesOf(actions), (std::vector{ActionType::ORDER_ADDED}));
}

} // namespace exchange::server
