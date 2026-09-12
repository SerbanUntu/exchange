#include "server/candlestick_publisher/candlestick_publisher.hpp"

namespace exchange::server
{
using namespace std::chrono;
using namespace std::chrono_literals;

int64_t CandlestickPublisher::timestampToIndex(const sys_seconds timestamp) const noexcept
{
    return (timestamp - inMemoryStart).count();
}

Candle &CandlestickPublisher::candleAt(const sys_seconds timestamp)
{
    return candles[timestampToIndex(timestamp)];
}

const Candle &CandlestickPublisher::candleAt(const sys_seconds timestamp) const
{
    return candles[timestampToIndex(timestamp)];
}

CandlestickPublisher::CandlestickPublisher()
    : candles{{.start{time_point_cast<seconds>(system_clock::now())}}}, inMemoryStart{candles[0].start}
{
}

void CandlestickPublisher::processTrade(const TradeAction &tradeAction, const sys_seconds timestamp) noexcept
{
    auto &lastCandle = candles[candles.size() - 1];
    if (timestamp > lastCandle.start)
    {
        lastCandle.isClosed = true;
        for (auto t = lastCandle.start + 1s; t < timestamp; t += 1s)
        {
            candles.emplace_back(Candle{.start = t, .isClosed = true});
        }
        candles.emplace_back(tradeAction.price, tradeAction.price, tradeAction.price, tradeAction.price,
                             tradeAction.quantity, timestamp, 1s);
    }
    else
    {
        if (lastCandle.empty())
            lastCandle.open = tradeAction.price;
        lastCandle.high = std::max(lastCandle.high, tradeAction.price);
        lastCandle.low = std::min(lastCandle.low, tradeAction.price);
        lastCandle.close = tradeAction.price;
        lastCandle.volume += tradeAction.quantity;
    }
}

std::vector<Candle> CandlestickPublisher::getCandles(const sys_seconds start, const sys_seconds end,
                                                     const seconds resolution) const
{
    if (timestampToIndex(start) >= static_cast<int64_t>(candles.size()))
        return {};
    std::vector<Candle> result;

    for (auto candleStart = start; candleStart < end; candleStart += resolution)
    {
        Candle fullResolutionCandle{.start{candleStart}, .resolution{resolution}, .isClosed = true};
        bool isOpenSet = false;
        for (auto t = candleStart; t < candleStart + resolution; t += 1s)
        {
            if (t < inMemoryStart)
                continue;
            const auto &currentCandle = candleAt(t);
            if (!currentCandle.isClosed)
                return result;
            if (currentCandle.empty())
                continue;
            if (!isOpenSet)
            {
                fullResolutionCandle.open = currentCandle.open;
                isOpenSet = true;
            }
            fullResolutionCandle.close = currentCandle.close;
            fullResolutionCandle.high = std::max(fullResolutionCandle.high, currentCandle.high);
            fullResolutionCandle.low = std::min(fullResolutionCandle.low, currentCandle.low);
            fullResolutionCandle.volume += currentCandle.volume;
        }
        if (!fullResolutionCandle.empty())
        {
            result.emplace_back(fullResolutionCandle);
        }
    }
    return result;
}
} // namespace exchange::server