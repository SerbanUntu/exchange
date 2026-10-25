#include "server/candlestick_publisher/candlestick_publisher.hpp"

#include "server/candlestick_publisher/model/candle.hpp"
#include "server/common/collections/ipc/action_spmc_receiver.hpp"
#include "server/common/collections/ipc/action_spmc_sender.hpp"
#include "server/common/collections/ipc/shared_memory.hpp"
#include "server/common/model/action/action.hpp"
#include "server/common/model/action/order_removed_action.hpp"
#include "server/common/model/action/trade_action.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace exchange::server
{
using namespace std::chrono;
using namespace std::chrono_literals;

namespace
{
constexpr SecurityId SECURITY{1};
constexpr SecurityId OTHER_SECURITY{2};

void expectCandle(const Candle &actual, const Candle &expected)
{
    EXPECT_EQ(actual.open, expected.open);
    EXPECT_EQ(actual.high, expected.high);
    EXPECT_EQ(actual.low, expected.low);
    EXPECT_EQ(actual.close, expected.close);
    EXPECT_EQ(actual.volume, expected.volume);
    EXPECT_EQ(actual.start.time_since_epoch().count(), expected.start.time_since_epoch().count());
    EXPECT_EQ(actual.resolution.count(), expected.resolution.count());
    EXPECT_EQ(actual.isClosed, expected.isClosed);
}
} // namespace

class CandlestickPublisherTest : public testing::Test
{
  protected:
    SharedMemory sharedMemory{true, "TestCandlestickPublisherSharedMemory"};
    std::optional<CandlestickPublisher> publisher;
    sys_seconds historyStart;

    CandlestickPublisherTest()
    {
        sharedMemory.get().construct<std::array<std::byte, SharedMemory::spmcTotalObjectSize>>(
            SharedMemory::spmcName)();
        do
        {
            historyStart = time_point_cast<seconds>(system_clock::now());
            publisher.emplace(ActionSpmcReceiver{sharedMemory});
        } while (time_point_cast<seconds>(system_clock::now()) != historyStart);
    }

    void trade(const int64_t secondsAfterStart, const uint64_t price, const uint64_t quantity,
               const SecurityId securityId = SECURITY)
    {
        publisher->processTrade(TradeAction{.price = Price{price}, .quantity = Quantity{quantity}},
                                historyStart + seconds{secondsAfterStart}, securityId);
    }

    [[nodiscard]] std::vector<Candle> candles(const int64_t startAfterHistory, const int64_t endAfterHistory,
                                              const seconds resolution, const SecurityId securityId = SECURITY) const
    {
        return publisher->getCandles(historyStart + seconds{startAfterHistory}, historyStart + seconds{endAfterHistory},
                                     resolution, securityId);
    }

    [[nodiscard]] Candle closedCandle(const int64_t startAfterHistory, const seconds resolution, const uint64_t open,
                                      const uint64_t high, const uint64_t low, const uint64_t close,
                                      const uint64_t volume) const
    {
        return Candle{.open = Price{open},
                      .high = Price{high},
                      .low = Price{low},
                      .close = Price{close},
                      .volume = Quantity{volume},
                      .start = historyStart + seconds{startAfterHistory},
                      .resolution = resolution,
                      .isClosed = true};
    }
};

TEST_F(CandlestickPublisherTest, GetCandlesReturnsNothingForAnUntradedSecurity)
{
    trade(0, 100, 1);
    trade(1, 100, 1);

    EXPECT_TRUE(candles(0, 1, 1s, OTHER_SECURITY).empty());
}

TEST_F(CandlestickPublisherTest, GetCandlesAggregatesClosedSecondsIntoOneCandle)
{
    trade(0, 100, 1);
    trade(0, 105, 2);
    trade(1, 95, 3);
    trade(2, 102, 1);
    trade(5, 200, 1);

    const auto result = candles(0, 3, 3s);

    ASSERT_EQ(result.size(), 1U);
    expectCandle(result[0], closedCandle(0, 3s, 100, 105, 95, 102, 7));
}

TEST_F(CandlestickPublisherTest, GetCandlesIgnoresSecondsWithoutTradesInsideACandle)
{
    trade(1, 100, 2);
    trade(3, 90, 4);
    trade(5, 200, 1);

    const auto result = candles(0, 5, 5s);

    ASSERT_EQ(result.size(), 1U);
    expectCandle(result[0], closedCandle(0, 5s, 100, 100, 90, 90, 6));
}

TEST_F(CandlestickPublisherTest, GetCandlesIncludesTheWholeCandleContainingTheEnd)
{
    trade(0, 100, 1);
    trade(2, 110, 1);
    trade(3, 200, 1);

    const auto result = candles(0, 1, 3s);

    ASSERT_EQ(result.size(), 1U);
    expectCandle(result[0], closedCandle(0, 3s, 100, 110, 100, 110, 2));
}

TEST_F(CandlestickPublisherTest, GetCandlesReturnsConsecutiveCandlesOnTheQueryGrid)
{
    trade(0, 100, 1);
    trade(2, 101, 1);
    trade(4, 102, 1);
    trade(6, 200, 1);

    const auto result = candles(0, 6, 2s);

    ASSERT_EQ(result.size(), 3U);
    expectCandle(result[0], closedCandle(0, 2s, 100, 100, 100, 100, 1));
    expectCandle(result[1], closedCandle(2, 2s, 101, 101, 101, 101, 1));
    expectCandle(result[2], closedCandle(4, 2s, 102, 102, 102, 102, 1));
}

TEST_F(CandlestickPublisherTest, GetCandlesDropsIntervalsWithoutTrades)
{
    trade(0, 100, 1);
    trade(10, 200, 1);

    const auto result = candles(0, 10, 2s);

    ASSERT_EQ(result.size(), 1U);
    expectCandle(result[0], closedCandle(0, 2s, 100, 100, 100, 100, 1));
}

TEST_F(CandlestickPublisherTest, GetCandlesDropsTheCandleThatIsStillOpen)
{
    trade(0, 100, 1);
    trade(1, 101, 1);

    EXPECT_TRUE(candles(0, 2, 2s).empty());

    const auto result = candles(0, 2, 1s);

    ASSERT_EQ(result.size(), 1U);
    expectCandle(result[0], closedCandle(0, 1s, 100, 100, 100, 100, 1));
}

TEST_F(CandlestickPublisherTest, GetCandlesDropsAnIntervalThatStartsBeforeRetainedHistory)
{
    trade(0, 100, 5);
    trade(40, 110, 2);
    trade(100, 200, 1);

    const auto result = candles(-30, 90, 60s);

    ASSERT_EQ(result.size(), 1U);
    expectCandle(result[0], closedCandle(30, 60s, 110, 110, 110, 110, 2));
}

TEST_F(CandlestickPublisherTest, GetCandlesStartingLongBeforeRetainedHistoryKeepsTheQueryGrid)
{
    trade(0, 100, 1);
    trade(1, 200, 1);

    const auto result = candles(-3600, 2, 1s);

    ASSERT_EQ(result.size(), 1U);
    expectCandle(result[0], closedCandle(0, 1s, 100, 100, 100, 100, 1));
}

TEST_F(CandlestickPublisherTest, GetCandlesReturnsNothingWhenTheFirstRetainedIntervalIsNotRecordedYet)
{
    trade(0, 100, 1);
    trade(3, 200, 1);

    EXPECT_TRUE(candles(-100, 200, 60s).empty());
}

TEST_F(CandlestickPublisherTest, GetCandlesReturnsNothingWhenStartIsAfterRecordedHistory)
{
    trade(0, 100, 1);
    trade(3, 200, 1);

    EXPECT_TRUE(candles(100, 200, 1s).empty());
}

TEST_F(CandlestickPublisherTest, ProcessTradeKeepsCandlesPerSecurity)
{
    trade(0, 100, 1, SECURITY);
    trade(0, 200, 3, OTHER_SECURITY);
    trade(1, 101, 1, SECURITY);
    trade(1, 201, 1, OTHER_SECURITY);

    const auto security = candles(0, 1, 1s, SECURITY);
    const auto otherSecurity = candles(0, 1, 1s, OTHER_SECURITY);

    ASSERT_EQ(security.size(), 1U);
    expectCandle(security[0], closedCandle(0, 1s, 100, 100, 100, 100, 1));
    ASSERT_EQ(otherSecurity.size(), 1U);
    expectCandle(otherSecurity[0], closedCandle(0, 1s, 200, 200, 200, 200, 3));
}

TEST_F(CandlestickPublisherTest, ProcessTradeDoesNotCloseCandlesOfOtherSecurities)
{
    trade(0, 100, 1, SECURITY);
    trade(5, 200, 1, OTHER_SECURITY);

    EXPECT_TRUE(candles(0, 1, 1s, SECURITY).empty());
}

TEST_F(CandlestickPublisherTest, LoopProcessesReceivedTradesAndIgnoresOtherActions)
{
    ActionSpmcSender sender{sharedMemory};
    sender.send(Action{Action::Header{.securityId = SECURITY, .timestamp = historyStart + 500ms},
                       TradeAction{.price = Price{100}, .quantity = Quantity{4}}});
    sender.send(
        Action{Action::Header{.securityId = SECURITY, .timestamp = historyStart + 600ms}, OrderRemovedAction{}});
    sender.send(Action{Action::Header{.securityId = SECURITY, .timestamp = historyStart + 1s},
                       TradeAction{.price = Price{200}, .quantity = Quantity{1}}});

    publisher->loop();
    publisher->loop();
    publisher->loop();

    const auto result = candles(0, 1, 1s);

    ASSERT_EQ(result.size(), 1U);
    expectCandle(result[0], closedCandle(0, 1s, 100, 100, 100, 100, 4));
}

} // namespace exchange::server
