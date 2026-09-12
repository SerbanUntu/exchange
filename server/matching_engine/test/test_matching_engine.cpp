#include "server/matching_engine/matching_engine.hpp"

#include "server/common/model/action/order_added_action.hpp"
#include "server/common/model/action/order_downsized_action.hpp"
#include "server/common/model/action/order_removed_action.hpp"
#include "server/common/model/action/order_upsized_action.hpp"
#include "server/common/model/action/trade_action.hpp"
#include "server/common/model/event/amend_limit_order_event.hpp"
#include "server/common/model/event/cancel_limit_order_event.hpp"
#include "server/common/model/event/new_limit_order_event.hpp"
#include "server/common/model/event/new_market_order_event.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <variant>
#include <vector>

namespace exchange::server
{
namespace
{
constexpr SecurityId SECURITY{1};
constexpr SecurityId OTHER_SECURITY{2};
constexpr std::array SECURITIES{SECURITY, OTHER_SECURITY};
const AccountId ACCOUNT{1};

template <typename T, typename Variant> struct VariantIndex;

template <typename T, typename... Rest>
struct VariantIndex<T, std::variant<T, Rest...>> : std::integral_constant<std::size_t, 0>
{
};

template <typename T, typename First, typename... Rest>
struct VariantIndex<T, std::variant<First, Rest...>>
    : std::integral_constant<std::size_t, 1 + VariantIndex<T, std::variant<Rest...>>::value>
{
};

template <typename T> constexpr std::size_t actionIndex = VariantIndex<T, decltype(Action::body)>::value;

constexpr std::size_t ORDER_ADDED = actionIndex<OrderAddedAction>;
constexpr std::size_t ORDER_DOWNSIZED = actionIndex<OrderDownsizedAction>;
constexpr std::size_t ORDER_REMOVED = actionIndex<OrderRemovedAction>;
constexpr std::size_t ORDER_UPSIZED = actionIndex<OrderUpsizedAction>;
constexpr std::size_t TRADE = actionIndex<TradeAction>;

OrderId makeOrderId(const uint8_t seed)
{
    boost::uuids::uuid uuid{};
    uuid.data[0] = seed;
    return OrderId{uuid};
}

template <typename Body>
Event makeEvent(const OrderId orderId, const Body &body, const SecurityId securityId = SECURITY)
{
    return Event{Event::Header{.orderId = orderId, .securityId = securityId, .accountId = ACCOUNT}, body};
}

std::vector<std::size_t> typesOf(const std::vector<Action> &actions)
{
    std::vector<std::size_t> types;
    types.reserve(actions.size());
    for (const auto &action : actions)
    {
        types.push_back(action.body.index());
    }
    return types;
}
} // namespace

class MatchingEngineTest : public testing::Test
{
  protected:
    MPSCQueue<Event> inQueue;
    SPMCQueue<Action> outQueue;
    MatchingEngine engine{inQueue, outQueue, SECURITIES};

    std::vector<Action> restGtc(const OrderId orderId, const Side side, const uint64_t price, const uint64_t quantity,
                                const SecurityId securityId = SECURITY)
    {
        return engine.processEvent(
            makeEvent(orderId,
                      NewLimitOrderEvent{
                          .side = side, .quantity = Quantity{quantity}, .price = Price{price}, .tif = TimeInForce::GTC},
                      securityId));
    }
};

TEST_F(MatchingEngineTest, ProcessEventDoesNotKeepNonGTCOrdersOnTheBook)
{
    const auto aggressor = makeOrderId(2);
    restGtc(makeOrderId(1), Side::SELL, 100, 2);

    const auto actions = engine.processEvent(makeEvent(
        aggressor,
        NewLimitOrderEvent{.side = Side::BUY, .quantity = Quantity{5}, .price = Price{100}, .tif = TimeInForce::IOC}));

    EXPECT_EQ(typesOf(actions), (std::vector{TRADE, ORDER_REMOVED}));

    EXPECT_TRUE(engine.processEvent(makeEvent(aggressor, CancelLimitOrderEvent{})).empty());
    EXPECT_EQ(typesOf(restGtc(makeOrderId(3), Side::SELL, 100, 3)), (std::vector{ORDER_ADDED}));
}

TEST_F(MatchingEngineTest, ProcessEventAddsAGtcLimitOrderToTheBook)
{
    const auto orderId = makeOrderId(1);

    const auto actions = restGtc(orderId, Side::BUY, 100, 5);

    ASSERT_EQ(actions.size(), 1U);
    EXPECT_EQ(actions[0].header.orderId, orderId);
    const auto &added = std::get<OrderAddedAction>(actions[0].body);
    EXPECT_EQ(added.price, Price{100});
    EXPECT_EQ(added.totalQuantity, Quantity{5});
    EXPECT_EQ(added.filledQuantity, Quantity{0});
    EXPECT_EQ(added.side, Side::BUY);
}

TEST_F(MatchingEngineTest, ProcessEventEmitsATradeAndRemovesTheFilledRestingOrder)
{
    const auto resting = makeOrderId(1);
    const auto aggressor = makeOrderId(2);
    restGtc(resting, Side::SELL, 100, 5);

    const auto actions = restGtc(aggressor, Side::BUY, 100, 5);

    ASSERT_EQ(typesOf(actions), (std::vector{TRADE, ORDER_REMOVED}));
    EXPECT_EQ(actions[0].header.orderId, aggressor);
    const auto &trade = std::get<TradeAction>(actions[0].body);
    EXPECT_EQ(trade.restingId, resting);
    EXPECT_EQ(trade.price, Price{100});
    EXPECT_EQ(trade.quantity, Quantity{5});
    EXPECT_EQ(trade.aggressorSide, Side::BUY);
    EXPECT_EQ(actions[1].header.orderId, resting);
}

TEST_F(MatchingEngineTest, ProcessEventDoesNotRemoveAPartiallyFilledRestingOrder)
{
    const auto resting = makeOrderId(1);
    restGtc(resting, Side::SELL, 100, 10);

    const auto actions = engine.processEvent(makeEvent(
        makeOrderId(2),
        NewLimitOrderEvent{.side = Side::BUY, .quantity = Quantity{4}, .price = Price{100}, .tif = TimeInForce::IOC}));

    EXPECT_EQ(typesOf(actions), (std::vector{TRADE}));
}

TEST_F(MatchingEngineTest, ProcessEventAddsTheUnfilledRemainderOfAGtcOrder)
{
    const auto aggressor = makeOrderId(2);
    restGtc(makeOrderId(1), Side::SELL, 100, 3);

    const auto actions = restGtc(aggressor, Side::BUY, 100, 10);

    ASSERT_EQ(typesOf(actions), (std::vector{TRADE, ORDER_REMOVED, ORDER_ADDED}));
    EXPECT_EQ(actions[2].header.orderId, aggressor);
    const auto &added = std::get<OrderAddedAction>(actions[2].body);
    EXPECT_EQ(added.totalQuantity, Quantity{10});
    EXPECT_EQ(added.filledQuantity, Quantity{3});
}

TEST_F(MatchingEngineTest, ProcessEventEmitsTradesForAMarketOrderWithoutAddingIt)
{
    const auto resting = makeOrderId(1);
    const auto aggressor = makeOrderId(2);
    restGtc(resting, Side::SELL, 100, 5);

    const auto actions =
        engine.processEvent(makeEvent(aggressor, NewMarketOrderEvent{.side = Side::BUY, .quantity = Quantity{5}}));

    ASSERT_EQ(typesOf(actions), (std::vector{TRADE, ORDER_REMOVED}));
    EXPECT_EQ(std::get<TradeAction>(actions[0].body).price, Price{100});
    EXPECT_EQ(actions[1].header.orderId, resting);
}

TEST_F(MatchingEngineTest, ProcessEventEmitsNoActionsForAMarketOrderWithoutLiquidity)
{
    const auto event = makeEvent(makeOrderId(1), NewMarketOrderEvent{.side = Side::BUY, .quantity = Quantity{5}});

    EXPECT_TRUE(engine.processEvent(event).empty());
}

TEST_F(MatchingEngineTest, ProcessEventEmitsNoActionsForAnUnfillableFokOrder)
{
    restGtc(makeOrderId(1), Side::SELL, 100, 2);

    const auto event = makeEvent(
        makeOrderId(2),
        NewLimitOrderEvent{.side = Side::BUY, .quantity = Quantity{5}, .price = Price{100}, .tif = TimeInForce::FOK});

    EXPECT_TRUE(engine.processEvent(event).empty());
}

TEST_F(MatchingEngineTest, ProcessEventDownsizesAnOrder)
{
    const auto orderId = makeOrderId(1);
    restGtc(orderId, Side::SELL, 100, 10);

    const auto actions = engine.processEvent(
        makeEvent(orderId, AmendLimitOrderEvent{.quantity = Quantity{4}, .isQuantityAmended = true}));

    ASSERT_EQ(typesOf(actions), (std::vector{ORDER_DOWNSIZED}));
    EXPECT_EQ(actions[0].header.orderId, orderId);
    EXPECT_EQ(std::get<OrderDownsizedAction>(actions[0].body).newQuantity, Quantity{4});
}

TEST_F(MatchingEngineTest, ProcessEventUpsizesAnOrder)
{
    const auto orderId = makeOrderId(1);
    restGtc(orderId, Side::SELL, 100, 10);

    const auto actions = engine.processEvent(
        makeEvent(orderId, AmendLimitOrderEvent{.quantity = Quantity{15}, .isQuantityAmended = true}));

    ASSERT_EQ(typesOf(actions), (std::vector{ORDER_UPSIZED}));
    EXPECT_EQ(actions[0].header.orderId, orderId);
    EXPECT_EQ(std::get<OrderUpsizedAction>(actions[0].body).newQuantity, Quantity{15});
}

TEST_F(MatchingEngineTest, ProcessEventRemovesAnOrderAmendedDownToItsFilledQuantity)
{
    const auto resting = makeOrderId(1);
    restGtc(resting, Side::SELL, 100, 10);
    restGtc(makeOrderId(2), Side::BUY, 100, 4);

    const auto actions = engine.processEvent(
        makeEvent(resting, AmendLimitOrderEvent{.quantity = Quantity{4}, .isQuantityAmended = true}));

    ASSERT_EQ(typesOf(actions), (std::vector{ORDER_REMOVED}));
    EXPECT_EQ(actions[0].header.orderId, resting);
}

TEST_F(MatchingEngineTest, ProcessEventEmitsNoActionsForARejectedAmend)
{
    const auto unknownOrder =
        makeEvent(makeOrderId(1), AmendLimitOrderEvent{.quantity = Quantity{5}, .isQuantityAmended = true});
    EXPECT_TRUE(engine.processEvent(unknownOrder).empty());

    const auto orderId = makeOrderId(2);
    restGtc(orderId, Side::SELL, 100, 10);
    const auto unchangedQuantity =
        makeEvent(orderId, AmendLimitOrderEvent{.quantity = Quantity{10}, .isQuantityAmended = true});
    EXPECT_TRUE(engine.processEvent(unchangedQuantity).empty());
}

TEST_F(MatchingEngineTest, ProcessEventRepricesAnOrderAsARemovalAndAnAddition)
{
    const auto orderId = makeOrderId(1);
    restGtc(orderId, Side::BUY, 100, 5);

    const auto actions =
        engine.processEvent(makeEvent(orderId, AmendLimitOrderEvent{.price = Price{99}, .isPriceAmended = true}));

    ASSERT_EQ(typesOf(actions), (std::vector{ORDER_REMOVED, ORDER_ADDED}));
    EXPECT_EQ(actions[0].header.orderId, orderId);
    EXPECT_EQ(actions[1].header.orderId, orderId);
    const auto &added = std::get<OrderAddedAction>(actions[1].body);
    EXPECT_EQ(added.price, Price{99});
    EXPECT_EQ(added.totalQuantity, Quantity{5});
}

TEST_F(MatchingEngineTest, ProcessEventEmitsTradesWhenAnAmendedPriceCrosses)
{
    const auto counterparty = makeOrderId(1);
    const auto orderId = makeOrderId(2);
    restGtc(counterparty, Side::SELL, 105, 5);
    restGtc(orderId, Side::BUY, 100, 5);

    const auto actions =
        engine.processEvent(makeEvent(orderId, AmendLimitOrderEvent{.price = Price{105}, .isPriceAmended = true}));

    ASSERT_EQ(typesOf(actions), (std::vector{ORDER_REMOVED, TRADE, ORDER_REMOVED}));
    EXPECT_EQ(actions[0].header.orderId, orderId);
    EXPECT_EQ(actions[1].header.orderId, orderId);
    const auto &trade = std::get<TradeAction>(actions[1].body);
    EXPECT_EQ(trade.restingId, counterparty);
    EXPECT_EQ(trade.price, Price{105});
    EXPECT_EQ(actions[2].header.orderId, counterparty);
}

TEST_F(MatchingEngineTest, ProcessEventDownsizesAnOrderWhenTheAmendRestatesTheCurrentPrice)
{
    const auto orderId = makeOrderId(1);
    restGtc(orderId, Side::SELL, 100, 10);

    const auto actions = engine.processEvent(makeEvent(
        orderId, AmendLimitOrderEvent{
                     .quantity = Quantity{4}, .price = Price{100}, .isQuantityAmended = true, .isPriceAmended = true}));

    ASSERT_EQ(typesOf(actions), (std::vector{ORDER_DOWNSIZED}));
    EXPECT_EQ(std::get<OrderDownsizedAction>(actions[0].body).newQuantity, Quantity{4});
}

TEST_F(MatchingEngineTest, ProcessEventCancelsARestingOrder)
{
    const auto orderId = makeOrderId(1);
    restGtc(orderId, Side::SELL, 100, 5);

    const auto actions = engine.processEvent(makeEvent(orderId, CancelLimitOrderEvent{}));

    ASSERT_EQ(typesOf(actions), (std::vector{ORDER_REMOVED}));
    EXPECT_EQ(actions[0].header.orderId, orderId);
    EXPECT_EQ(typesOf(restGtc(makeOrderId(2), Side::BUY, 100, 5)), (std::vector{ORDER_ADDED}));
}

TEST_F(MatchingEngineTest, ProcessEventEmitsNoActionsWhenCancellingAnUnknownOrder)
{
    const auto event = makeEvent(makeOrderId(1), CancelLimitOrderEvent{});

    EXPECT_TRUE(engine.processEvent(event).empty());
}

TEST_F(MatchingEngineTest, ProcessEventKeepsAnOrderBookPerSecurity)
{
    restGtc(makeOrderId(1), Side::SELL, 100, 5, SECURITY);

    const auto actions = restGtc(makeOrderId(2), Side::BUY, 100, 5, OTHER_SECURITY);

    EXPECT_EQ(typesOf(actions), (std::vector{ORDER_ADDED}));
}

} // namespace exchange::server
