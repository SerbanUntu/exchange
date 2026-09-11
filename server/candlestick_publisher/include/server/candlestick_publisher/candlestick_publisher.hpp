#pragma once
#include "server/candlestick_publisher/model/candle.hpp"
#include "server/common/model/action/trade_action.hpp"
#include <chrono>
#include <vector>

namespace exchange::server
{
class CandlestickPublisher
{
    std::vector<Candle> candles;
    std::chrono::sys_seconds inMemoryStart;

    [[nodiscard]] int64_t timestampToIndex(std::chrono::sys_seconds timestamp) const noexcept;
    [[nodiscard]] Candle &candleAt(std::chrono::sys_seconds timestamp);
    [[nodiscard]] const Candle &candleAt(std::chrono::sys_seconds timestamp) const;

  public:
    CandlestickPublisher();
    /**
     * Update second-precision candles with the data from an executed trade.
     *
     * Assume trades arrive in non-decreasing order of timestamp and no trades are executed before the CandlePublisher
     * is constructed.
     *
     * Invariant: The only candle that is not closed is the most recent one.
     *
     * @param tradeAction The data corresponding to a single trade, needed to process.
     * @param timestamp The moment when the trade occurred.
     */
    void processTrade(const TradeAction &tradeAction, std::chrono::sys_seconds timestamp) noexcept;

    /**
     * Compute a list of candles within a time range, at arbitrary resolution.
     *
     * Assumes the input is valid (start < end, resolution >= 1).
     *
     * All candles that would contain partial/no data are dropped from the result.
     * If the end timestamp is within a closed candle (it does not align with the grid), the whole time interval of
     * that candle is then considered, and the full candle is included in the final result, even though it might exceed
     * the specified end time.
     *
     * @param start The timestamp that indicates the start time of the first candle, inclusive
     * @param end The timestamp that is one over the last timestamp at which a candle can start
     * @param resolution Over what period a single candle is calculated, in seconds
     * @return The list of valid candles in the specified interval. If the input is invalid, an empty vector is
     * returned.
     */
    [[nodiscard]] std::vector<Candle> getCandles(std::chrono::sys_seconds start, std::chrono::sys_seconds end,
                                   std::chrono::seconds resolution) const;
};
} // namespace exchange::server