#include "client/response_displayer.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace exchange::client
{
namespace
{
const std::string ORDER_ID = "123e4567-e89b-12d3-a456-426614174000";
const std::string RESTING_ID = "987e6543-e21b-12d3-a456-426614174999";
const std::string THIRD_ID = "555e5555-e55b-55d5-a555-555555555555";
constexpr uint32_t SECURITY_ID = 4242;
const std::string SECURITY_ID_STRING = "4242";
constexpr int64_t T1_SECONDS = 1700000000;
constexpr int64_t T2_SECONDS = 1700000060;
constexpr int64_t T3_SECONDS = 1700000120;
const std::string T1_STRING = "2023-11-14T22:13:20Z";
const std::string T2_STRING = "2023-11-14T22:14:20Z";
const std::string T3_STRING = "2023-11-14T22:15:20Z";

using TokenGroups = std::vector<std::vector<std::string>>;

bool isWordChar(const char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) != 0;
}

std::size_t findToken(const std::string &text, const std::string &token, const std::size_t from)
{
    for (auto pos = text.find(token, from); pos != std::string::npos; pos = text.find(token, pos + 1))
    {
        const auto end = pos + token.size();
        const bool startsAtBoundary = pos == 0 || !isWordChar(token.front()) || !isWordChar(text[pos - 1]);
        const bool endsAtBoundary = end == text.size() || !isWordChar(token.back()) || !isWordChar(text[end]);
        if (startsAtBoundary && endsAtBoundary)
        {
            return pos;
        }
    }
    return std::string::npos;
}

bool containsToken(const std::string &text, const std::string &token)
{
    return findToken(text, token, 0) != std::string::npos;
}

testing::AssertionResult containsGroupsInOrder(const std::string &text, const TokenGroups &groups)
{
    std::size_t groupStart = 0;
    for (const auto &group : groups)
    {
        std::size_t groupEnd = groupStart;
        for (const auto &token : group)
        {
            const auto pos = findToken(text, token, groupStart);
            if (pos == std::string::npos)
            {
                return testing::AssertionFailure()
                       << "expected \"" << token << "\" at or after offset " << groupStart << " in:\n"
                       << text;
            }
            groupEnd = std::max(groupEnd, pos + token.size());
        }
        groupStart = groupEnd;
    }
    return testing::AssertionSuccess();
}

testing::AssertionResult containsInOrder(const std::string &text, const std::vector<std::string> &tokens)
{
    TokenGroups groups;
    for (const auto &token : tokens)
    {
        groups.push_back({token});
    }
    return containsGroupsInOrder(text, groups);
}

testing::AssertionResult containsAll(const std::string &text, const std::vector<std::string> &tokens)
{
    return containsGroupsInOrder(text, {tokens});
}

template <typename Response> std::string render(const Response &response)
{
    std::ostringstream out;
    const ResponseDisplayer displayer{out};
    displayer.printResponse(response);
    return out.str();
}

void setTimestamp(google::protobuf::Timestamp *timestamp, const int64_t seconds)
{
    timestamp->set_seconds(seconds);
    timestamp->set_nanos(0);
}

proto::OrderBookUpdate makeUpdate()
{
    proto::OrderBookUpdate update;
    update.mutable_header()->set_order_id(ORDER_ID);
    update.mutable_header()->set_security_id(SECURITY_ID);
    setTimestamp(update.mutable_header()->mutable_timestamp(), T1_SECONDS);
    return update;
}

std::vector<std::string> headerTokens()
{
    return {ORDER_ID, SECURITY_ID_STRING, T1_STRING};
}

void addCandle(proto::CandleDataResponse &candles, const int64_t start, const int64_t end,
               const std::vector<uint64_t> &ohlcv)
{
    auto *candle = candles.add_candles();
    setTimestamp(candle->mutable_start_time(), start);
    setTimestamp(candle->mutable_end_time(), end);
    candle->set_opening_price(ohlcv[0]);
    candle->set_high_price(ohlcv[1]);
    candle->set_low_price(ohlcv[2]);
    candle->set_closing_price(ohlcv[3]);
    candle->set_total_quantity(ohlcv[4]);
}

void addLevel(google::protobuf::RepeatedPtrField<proto::L2Level> *levels, const uint64_t price, const uint64_t quantity)
{
    auto *level = levels->Add();
    level->set_price(price);
    level->set_quantity(quantity);
}

void addTrade(proto::Execution *execution, const int64_t time, const uint64_t price, const uint64_t quantity)
{
    auto *trade = execution->add_fill_result();
    setTimestamp(trade->mutable_execution_time(), time);
    trade->set_price(price);
    trade->set_quantity(quantity);
}

std::string renderPrice(const uint64_t price)
{
    proto::L2DataResponse l2Data;
    addLevel(l2Data.mutable_buy_levels(), price, 7777);
    return render(l2Data);
}

std::string renderOrderWithSide(const proto::Side side)
{
    proto::OrderDataResponse order;
    order.set_side(side);
    return render(order);
}

std::string renderOrderWithStatus(const proto::OrderStatus status)
{
    proto::OrderDataResponse order;
    order.set_order_status(status);
    return render(order);
}

proto::Security makeSecurity(const uint32_t id, const std::string &symbol, const std::string &name,
                             const std::string &description)
{
    proto::Security security;
    security.set_security_id(id);
    security.set_symbol(symbol);
    security.set_name(name);
    security.set_description(description);
    return security;
}
} // namespace

TEST(ResponseDisplayerTest, DefaultConstructorWritesToStdout)
{
    const ResponseDisplayer displayer;
    EXPECT_EQ(&displayer.getOutputStream(), &std::cout);
}

TEST(ResponseDisplayerTest, ExplicitStreamIsUsed)
{
    std::ostringstream out;
    const ResponseDisplayer displayer{out};
    EXPECT_EQ(&displayer.getOutputStream(), &out);
}

TEST(ResponseDisplayerTest, OrderAckShowsOrderId)
{
    proto::OrderAck ack;
    ack.set_order_id(ORDER_ID);

    EXPECT_TRUE(containsToken(render(ack), ORDER_ID));
}

TEST(ResponseDisplayerTest, CandleDataShowsEveryCandleInOrder)
{
    proto::CandleDataResponse candles;
    addCandle(candles, T1_SECONDS, T2_SECONDS, {10050, 10275, 9905, 10125, 1337});
    addCandle(candles, T2_SECONDS, T3_SECONDS, {20010, 20555, 19990, 20300, 2718});

    const auto output = render(candles);

    EXPECT_TRUE(
        containsGroupsInOrder(output, {{T1_STRING, T2_STRING, "100.50", "102.75", "99.05", "101.25", "1337"},
                                       {T2_STRING, T3_STRING, "200.10", "205.55", "199.90", "203.00", "2718"}}));
    EXPECT_TRUE(containsInOrder(output, {T1_STRING, T2_STRING, T3_STRING}));
}

TEST(ResponseDisplayerTest, L1DataShowsAllPricesAndQuantities)
{
    proto::L1DataResponse l1Data;
    l1Data.set_best_buy(9999);
    l1Data.set_best_buy_quantity(51);
    l1Data.set_best_sell(10001);
    l1Data.set_best_sell_quantity(62);
    l1Data.set_last_trade_price(10000);
    l1Data.set_last_trade_quantity(73);

    EXPECT_TRUE(containsAll(render(l1Data), {"99.99", "51", "100.01", "62", "100.00", "73"}));
}

TEST(ResponseDisplayerTest, L2DataShowsLevelsInOrderGroupedBySide)
{
    proto::L2DataResponse l2Data;
    addLevel(l2Data.mutable_buy_levels(), 10000, 31);
    addLevel(l2Data.mutable_buy_levels(), 9950, 82);
    addLevel(l2Data.mutable_sell_levels(), 10025, 47);
    addLevel(l2Data.mutable_sell_levels(), 10080, 59);

    const auto output = render(l2Data);
    const std::vector<std::string> buyTokens{"100.00", "31", "99.50", "82"};
    const std::vector<std::string> sellTokens{"100.25", "47", "100.80", "59"};

    EXPECT_TRUE(containsGroupsInOrder(output, {{"100.00", "31"}, {"99.50", "82"}}));
    EXPECT_TRUE(containsGroupsInOrder(output, {{"100.25", "47"}, {"100.80", "59"}}));
    EXPECT_TRUE(containsGroupsInOrder(output, {buyTokens, sellTokens}) ||
                containsGroupsInOrder(output, {sellTokens, buyTokens}));
}

TEST(ResponseDisplayerTest, L2DataWithOnlySellLevelsShowsThem)
{
    proto::L2DataResponse l2Data;
    addLevel(l2Data.mutable_sell_levels(), 10025, 47);

    EXPECT_TRUE(containsAll(render(l2Data), {"100.25", "47"}));
}

TEST(ResponseDisplayerTest, PricesAreFormattedAsUnitsAndCents)
{
    EXPECT_TRUE(containsToken(renderPrice(0), "0.00"));
    EXPECT_TRUE(containsToken(renderPrice(5), "0.05"));
    EXPECT_TRUE(containsToken(renderPrice(99), "0.99"));
    EXPECT_TRUE(containsToken(renderPrice(100), "1.00"));
    EXPECT_TRUE(containsToken(renderPrice(123456789), "1234567.89"));
}

TEST(ResponseDisplayerTest, OrderBookUpdateOrderAddedShowsDetails)
{
    auto update = makeUpdate();
    auto *added = update.mutable_order_added();
    added->set_side(proto::Side::SIDE_BUY);
    added->set_price(10050);
    added->set_filled_quantity(1337);
    added->set_total_quantity(2718);

    const auto output = render(update);

    EXPECT_TRUE(containsAll(output, headerTokens()));
    EXPECT_TRUE(containsAll(output, {"BUY", "100.50"}));
    EXPECT_TRUE(containsInOrder(output, {"1337", "2718"}));
}

TEST(ResponseDisplayerTest, OrderBookUpdateOrderDownsizedShowsNewQuantity)
{
    auto update = makeUpdate();
    update.mutable_order_downsized()->set_new_quantity(3141);

    const auto output = render(update);

    EXPECT_TRUE(containsAll(output, headerTokens()));
    EXPECT_TRUE(containsToken(output, "3141"));
}

TEST(ResponseDisplayerTest, OrderBookUpdateOrderRemovedShowsHeader)
{
    auto update = makeUpdate();
    update.mutable_order_removed();

    EXPECT_TRUE(containsAll(render(update), headerTokens()));
}

TEST(ResponseDisplayerTest, OrderBookUpdateOrderUpsizedShowsNewQuantity)
{
    auto update = makeUpdate();
    update.mutable_order_upsized()->set_new_quantity(3141);

    const auto output = render(update);

    EXPECT_TRUE(containsAll(output, headerTokens()));
    EXPECT_TRUE(containsToken(output, "3141"));
}

TEST(ResponseDisplayerTest, OrderBookUpdateTradeShowsDetails)
{
    auto update = makeUpdate();
    auto *trade = update.mutable_trade();
    trade->set_resting_id(RESTING_ID);
    trade->set_price(9900);
    trade->set_quantity(1618);
    trade->set_aggressor_side(proto::Side::SIDE_SELL);
    trade->set_remaining_resting_quantity(1414);

    const auto output = render(update);

    EXPECT_TRUE(containsAll(output, headerTokens()));
    EXPECT_TRUE(containsAll(output, {RESTING_ID, "99.00", "1618", "SELL", "1414"}));
}

TEST(ResponseDisplayerTest, OrderBookUpdateWithoutBodyShowsHeader)
{
    EXPECT_TRUE(containsAll(render(makeUpdate()), headerTokens()));
}

TEST(ResponseDisplayerTest, OrderBookUpdateKindsAreDistinguishable)
{
    auto added = makeUpdate();
    added.mutable_order_added();
    auto downsized = makeUpdate();
    downsized.mutable_order_downsized()->set_new_quantity(3141);
    auto removed = makeUpdate();
    removed.mutable_order_removed();
    auto upsized = makeUpdate();
    upsized.mutable_order_upsized()->set_new_quantity(3141);
    auto trade = makeUpdate();
    trade.mutable_trade();
    const auto empty = makeUpdate();

    const std::set<std::string> outputs{render(added),   render(downsized), render(removed),
                                        render(upsized), render(trade),     render(empty)};

    EXPECT_EQ(outputs.size(), 6U);
}

TEST(ResponseDisplayerTest, OrderDataShowsDetails)
{
    proto::OrderDataResponse order;
    order.set_order_id(ORDER_ID);
    order.set_side(proto::Side::SIDE_SELL);
    order.set_order_status(proto::OrderStatus::ORDER_STATUS_RESTING);
    order.set_price(12345);
    order.set_filled_quantity(1337);
    order.set_total_quantity(2718);

    const auto output = render(order);

    EXPECT_TRUE(containsAll(output, {ORDER_ID, "SELL", "RESTING", "123.45"}));
    EXPECT_TRUE(containsInOrder(output, {"1337", "2718"}));
}

TEST(ResponseDisplayerTest, SidesAreNamed)
{
    const auto buy = renderOrderWithSide(proto::Side::SIDE_BUY);
    const auto sell = renderOrderWithSide(proto::Side::SIDE_SELL);
    const auto unspecified = renderOrderWithSide(proto::Side::SIDE_UNSPECIFIED);

    EXPECT_TRUE(containsToken(buy, "BUY"));
    EXPECT_FALSE(containsToken(buy, "SELL"));
    EXPECT_TRUE(containsToken(sell, "SELL"));
    EXPECT_FALSE(containsToken(sell, "BUY"));
    EXPECT_FALSE(containsToken(unspecified, "BUY"));
    EXPECT_FALSE(containsToken(unspecified, "SELL"));
}

TEST(ResponseDisplayerTest, OrderStatusesAreNamed)
{
    const auto resting = renderOrderWithStatus(proto::OrderStatus::ORDER_STATUS_RESTING);
    const auto finished = renderOrderWithStatus(proto::OrderStatus::ORDER_STATUS_FINISHED);
    const auto unspecified = renderOrderWithStatus(proto::OrderStatus::ORDER_STATUS_UNSPECIFIED);

    EXPECT_TRUE(containsToken(resting, "RESTING"));
    EXPECT_FALSE(containsToken(resting, "FINISHED"));
    EXPECT_TRUE(containsToken(finished, "FINISHED"));
    EXPECT_FALSE(containsToken(finished, "RESTING"));
    EXPECT_FALSE(containsToken(unspecified, "RESTING"));
    EXPECT_FALSE(containsToken(unspecified, "FINISHED"));
}

TEST(ResponseDisplayerTest, ExecutionDataShowsExecutionsAndTradesInOrder)
{
    proto::ExecutionDataResponse executionData;
    auto *first = executionData.add_executions();
    first->set_order_id(ORDER_ID);
    addTrade(first, T1_SECONDS, 10000, 31);
    addTrade(first, T2_SECONDS, 10010, 59);
    auto *second = executionData.add_executions();
    second->set_order_id(RESTING_ID);
    addTrade(second, T3_SECONDS, 9990, 82);
    executionData.add_executions()->set_order_id(THIRD_ID);

    EXPECT_TRUE(containsGroupsInOrder(render(executionData), {{ORDER_ID},
                                                              {T1_STRING, "100.00", "31"},
                                                              {T2_STRING, "100.10", "59"},
                                                              {RESTING_ID},
                                                              {T3_STRING, "99.90", "82"},
                                                              {THIRD_ID}}));
}

TEST(ResponseDisplayerTest, SecurityShowsAllFields)
{
    const auto security = makeSecurity(SECURITY_ID, "ACME", "Acme Corporation", "Maker of anvils");

    EXPECT_TRUE(containsAll(render(security), {SECURITY_ID_STRING, "ACME", "Acme Corporation", "Maker of anvils"}));
}

TEST(ResponseDisplayerTest, SecurityListShowsEachSecurityInOrder)
{
    proto::ListSecuritiesResponse securities;
    *securities.add_securities() = makeSecurity(1, "AAA", "Alpha", "First");
    *securities.add_securities() = makeSecurity(2, "BBB", "Beta", "Second");

    EXPECT_TRUE(
        containsInOrder(render(securities), {render(securities.securities(0)), render(securities.securities(1))}));
}

} // namespace exchange::client
