#include "client/response_displayer.hpp"

#include <google/protobuf/util/time_util.h>

namespace exchange::client
{

std::string ResponseDisplayer::toString(const google::protobuf::Timestamp &timestamp)
{
    return google::protobuf::util::TimeUtil::ToString(timestamp);
}

std::string ResponseDisplayer::toString(const proto::Side side)
{
    switch (side)
    {
    case proto::Side::SIDE_BUY:
        return "BUY";
    case proto::Side::SIDE_SELL:
        return "SELL";
    default:
        return "UNSPECIFIED SIDE";
    }
}

std::string ResponseDisplayer::toString(const proto::OrderStatus status)
{
    switch (status)
    {
    case proto::OrderStatus::ORDER_STATUS_RESTING:
        return "RESTING";
    case proto::OrderStatus::ORDER_STATUS_FINISHED:
        return "FINISHED";
    default:
        return "UNSPECIFIED STATUS";
    }
}

std::string ResponseDisplayer::priceToString(const uint64_t price)
{
    std::string cents = std::to_string(price % 100);
    while (cents.size() < 2)
        cents.insert(0, 1, '0');
    return std::to_string(price / 100) + '.' + cents + " EUR";
}

ResponseDisplayer::ResponseDisplayer() : out{std::cout}
{
}

ResponseDisplayer::ResponseDisplayer(std::ostream &outputStream) : out{outputStream}
{
}

std::ostream &ResponseDisplayer::getOutputStream() const
{
    return out.get();
}

void ResponseDisplayer::printResponse(const proto::OrderAck &ack) const
{
    out.get() << "Your order request has been acknowledged and is queued for processing at the exchange.\n";
    out.get() << "Order ID: " << ack.order_id() << '\n';
}

void ResponseDisplayer::printResponse(const proto::CandleDataResponse &candles) const
{
    out.get() << "Candles (" << candles.candles_size() << "):\n\n";
    for (const auto &candle : candles.candles())
    {
        out.get() << "[" << toString(candle.start_time()) << ", " << toString(candle.end_time())
                  << "] O: " << priceToString(candle.opening_price()) << ", H: " << priceToString(candle.high_price())
                  << ", L: " << priceToString(candle.low_price()) << ", C: " << priceToString(candle.closing_price())
                  << ", V: " << candle.total_quantity() << '\n';
    }
}

void ResponseDisplayer::printResponse(const proto::L1DataResponse &l1Data) const
{
    out.get() << "Best buy: " << priceToString(l1Data.best_buy()) << ", " << l1Data.best_buy_quantity() << " units\n";
    out.get() << "Best sell: " << priceToString(l1Data.best_sell()) << ", " << l1Data.best_sell_quantity()
              << " units\n";
    out.get() << "Last trade: " << priceToString(l1Data.last_trade_price()) << ", " << l1Data.last_trade_quantity()
              << " units\n";
}

void ResponseDisplayer::printResponse(const proto::L2DataResponse &l2Data) const
{
    out.get() << "Buy levels (" << l2Data.buy_levels_size() << "):\n\n";
    for (int i{}; i < l2Data.buy_levels_size(); ++i)
    {
        out.get() << "Level " << i + 1 << ": " << priceToString(l2Data.buy_levels(i).price()) << ", "
                  << l2Data.buy_levels(i).quantity() << " units\n";
    }

    out.get() << "Sell levels (" << l2Data.sell_levels_size() << "):\n\n";
    for (int i{}; i < l2Data.sell_levels_size(); ++i)
    {
        out.get() << "Level " << i + 1 << ": " << priceToString(l2Data.sell_levels(i).price()) << ", "
                  << l2Data.sell_levels(i).quantity() << " units\n";
    }
}

void ResponseDisplayer::printResponse(const proto::OrderBookUpdate &update) const
{
    const auto &header = update.header();
    out.get() << "[" << toString(header.timestamp()) << "] Order " << header.order_id() << " (security "
              << header.security_id() << ") ";

    switch (update.body_case())
    {
    case proto::OrderBookUpdate::kOrderAdded: {
        const auto &added = update.order_added();
        out.get() << "Added: Side: " << toString(added.side()) << ", Price: " << priceToString(added.price())
                  << ", Filled Quantity: " << added.filled_quantity() << '/' << added.total_quantity() << '\n';
        break;
    }
    case proto::OrderBookUpdate::kOrderDownsized:
        out.get() << "Downsized: New Quantity: " << update.order_downsized().new_quantity() << '\n';
        break;
    case proto::OrderBookUpdate::kOrderRemoved:
        out.get() << "Removed\n";
        break;
    case proto::OrderBookUpdate::kOrderUpsized:
        out.get() << "Upsized: New Quantity: " << update.order_upsized().new_quantity() << '\n';
        break;
    case proto::OrderBookUpdate::kTrade: {
        const auto &trade = update.trade();
        out.get() << "Traded against resting order " << trade.resting_id() << ": " << priceToString(trade.price())
                  << ", " << trade.quantity() << " units, Aggressor: " << toString(trade.aggressor_side())
                  << ", Resting Quantity Remaining: " << trade.remaining_resting_quantity() << '\n';
        break;
    }
    default:
        out.get() << "Unknown update\n";
        break;
    }
}

void ResponseDisplayer::printResponse(const proto::OrderDataResponse &order) const
{
    out.get() << toString(order.side()) << " Order " << order.order_id() << " (" << toString(order.order_status())
              << "):\n";
    out.get() << "Price: " << priceToString(order.price()) << '\n';
    out.get() << "Filled Quantity: " << order.filled_quantity() << '/' << order.total_quantity() << '\n';
}

void ResponseDisplayer::printResponse(const proto::ExecutionDataResponse &executionData) const
{
    out.get() << "Executions (" << executionData.executions_size() << "):\n\n";
    for (const auto &execution : executionData.executions())
    {
        out.get() << "Order " << execution.order_id() << " (" << execution.fill_result_size() << " trades):\n";
        for (const auto &trade : execution.fill_result())
        {
            out.get() << "[" << toString(trade.execution_time()) << "] " << priceToString(trade.price()) << ", "
                      << trade.quantity() << " units\n";
        }
    }
}

void ResponseDisplayer::printResponse(const proto::ListSecuritiesResponse &securities) const
{
    out.get() << "Exchange securities (" << securities.securities_size() << "):\n\n";
    for (const auto &security : securities.securities())
    {
        printResponse(security);
    }
}

void ResponseDisplayer::printResponse(const proto::Security &security) const
{
    out.get() << security.symbol() << ": ID " << security.security_id() << ", Full Name: " << security.name()
              << ", Description: " << security.description() << ".\n";
}

} // namespace exchange::client