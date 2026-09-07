#include <server/matching_engine/main.hpp>

#include "common/util/logging.hpp"
#include "server/common/collections/orderbook/orderbook.hpp"
#include "server/common/model/action/action.hpp"
#include "server/common/model/action/order_added_action.hpp"
#include "server/common/model/action/order_executed_action.hpp"
#include "server/common/model/action/order_matched_action.hpp"
#include "server/common/model/event/event.hpp"
#include "server/common/model/event/new_limit_order_event.hpp"
#include "server/common/model/value_object/security_id.hpp"

#include <spdlog/logger.h>

namespace exchange::server
{

std::vector<std::unique_ptr<Action>> processEvent(const Event &event,
                                                  std::unordered_map<SecurityId, OrderBook> &state)
{
    auto &orderBook = state.at(event.orderId.securityId);
    auto result{std::vector<std::unique_ptr<Action>>()};

    switch (event.getType())
    {
    case EventType::NEW_LIMIT_ORDER: {
        const auto &e = dynamic_cast<const NewLimitOrderEvent &>(event);
        if (e.tif == TimeInForce::GTC)
        {
            orderBook.addOrder(e.orderId, e.accountId, e.side, e.price, e.quantity, e.tif);
            result.push_back(std::make_unique<OrderAddedAction>(e.orderId, e.accountId, e.price, e.quantity, e.tif));
        }
        const auto [matchedOrders, remainingQuantity]{orderBook.matchOrders(
            e.side == Side::BUY ? Side::SELL : Side::BUY, e.price, e.quantity, e.tif == TimeInForce::FOK)};
        for (size_t i{0}; i < matchedOrders.size(); ++i)
        {
            const auto &matchedOrder = matchedOrders[i];
            result.push_back(std::make_unique<OrderMatchedAction>(matchedOrder.orderId, matchedOrder.accountId,
                                                                  matchedOrder.matchedQuantity, matchedOrder.price));
            if (matchedOrder.isFullyMatched)
            {
                result.push_back(std::make_unique<OrderExecutedAction>(matchedOrder.orderId, matchedOrder.accountId));
            }
            result.push_back(std::make_unique<OrderMatchedAction>(e.orderId, e.accountId, e.quantity, e.price));
        }
        if (e.tif == TimeInForce::GTC)
        {
            if (remainingQuantity == Quantity(0))
            {
                orderBook.removeOrder(e.orderId, e.accountId);
            }
            else
            {
                orderBook.partiallyFillOrder(e.orderId, e.accountId, e.quantity - remainingQuantity);
            }
        }
        if (e.tif != TimeInForce::GTC || remainingQuantity == Quantity(0))
        {
            result.push_back(std::make_unique<OrderExecutedAction>(e.orderId, e.accountId));
        }
        return result;
    }
    }
}

int matchingEngineMain() noexcept
{
    using namespace exchange::common;
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = LogService::getLogger(LogProducer::MATCHING_ENGINE);
        LOG->info("Hello from the Matching Engine.");
        return 0;
    }
    catch (const std::exception &err)
    {
        reportException(LOG, err);
        return 1;
    }
    catch (...)
    {
        reportUnknownException(LOG);
        return 1;
    }
}
} // namespace exchange::server

int main()
{
    return exchange::server::matchingEngineMain();
}
