#include "server/candlestick_publisher/candlestick_publisher.hpp"

#include "common/util/overloads.hpp"

namespace exchange::server
{
using namespace std::chrono;
using namespace std::chrono_literals;

int64_t CandlestickPublisher::timestampToIndex(const sys_seconds timestamp) const noexcept
{
    return (timestamp - inMemoryStart).count();
}

CandlestickPublisher::CandlestickPublisher(ActionSpmcReceiver &&actionReceiver)
    : actionReceiver(std::move(actionReceiver)), inMemoryStart{time_point_cast<seconds>(system_clock::now())}
{
}

void CandlestickPublisher::loop()
{
    const auto action = actionReceiver.receive();

    std::visit(overloads{[&](const TradeAction &tradeAction) {
                             processTrade(tradeAction, time_point_cast<seconds>(action.header.timestamp),
                                          action.header.securityId);
                         },
                         [](const auto &) {}},
               action.body);
}

void CandlestickPublisher::processTrade(const TradeAction &tradeAction, const sys_seconds timestamp,
                                        const SecurityId securityId)
{
    auto [it, _] = state.try_emplace(securityId, 1, Candle{.start = inMemoryStart});
    auto &candles = it->second;
    auto &lastCandle = candles.back();
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
                                                     const seconds resolution, const SecurityId securityId) const
{
    const auto it = state.find(securityId);
    if (it == state.end())
        return {};
    const auto &candles = it->second;

    auto firstCandleStart = start;
    if (firstCandleStart < inMemoryStart)
        firstCandleStart += (inMemoryStart - start + resolution - 1s) / resolution * resolution;

    std::vector<Candle> result;
    for (auto candleStart = firstCandleStart; candleStart < end; candleStart += resolution)
    {
        if (timestampToIndex(candleStart) >= static_cast<int64_t>(candles.size()))
            break;
        Candle fullResolutionCandle{.start{candleStart}, .resolution{resolution}, .isClosed = true};
        bool isOpenSet = false;
        for (auto t = candleStart; t < candleStart + resolution; t += 1s)
        {
            const auto &currentCandle = candles[timestampToIndex(t)];
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