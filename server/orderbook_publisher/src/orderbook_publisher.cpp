#include "server/orderbook_publisher/orderbook_publisher.hpp"

#include "common/util/overloads.hpp"

#include <algorithm>

namespace exchange::server
{

OrderbookPublisher::OrderbookPublisher(ActionSpmcReceiver &&actionReceiver) : actionReceiver(std::move(actionReceiver))
{
}

void OrderbookPublisher::loop()
{
    processAction(actionReceiver.receive());
}

void OrderbookPublisher::processAction(const Action &action)
{
    auto &securityState = state[action.header.securityId];
    std::visit(overloads{
                   [&](const OrderAddedAction &actionBody) {
                       securityState.orderBook.addOrder(action.header.orderId, actionBody.side, actionBody.price,
                                                        actionBody.totalQuantity, actionBody.filledQuantity,
                                                        TimeInForce::GTC, OrderType::LIMIT);
                   },
                   [&](const OrderDownsizedAction &actionBody) {
                       securityState.orderBook.amendOrder(action.header.orderId, actionBody.newQuantity);
                   },
                   [&](const OrderRemovedAction &_) { securityState.orderBook.removeOrder(action.header.orderId); },
                   [&](const OrderUpsizedAction &actionBody) {
                       securityState.orderBook.amendOrder(action.header.orderId, actionBody.newQuantity);
                   },
                   [&](const TradeAction &actionBody) {
                       securityState.lastTradePrice = actionBody.price;
                       securityState.lastTradeQuantity = actionBody.quantity;
                       const auto maybeRestingOrder = securityState.orderBook.getOrder(actionBody.restingId);
                       if (maybeRestingOrder != std::nullopt)
                       {
                           const auto restingOrder = *maybeRestingOrder;
                           securityState.orderBook.removeOrder(restingOrder.id);
                           securityState.orderBook.addOrder(
                               restingOrder.id, restingOrder.side, restingOrder.price, restingOrder.totalQuantity,
                               restingOrder.totalQuantity - actionBody.remainingRestingQuantity, TimeInForce::GTC,
                               OrderType::LIMIT);
                       }
                   },
               },
               action.body);
}

L1Data OrderbookPublisher::getL1Data(const SecurityId securityId) const
{
    const auto it = state.find(securityId);
    if (it == state.end())
    {
        return {};
    }

    const auto &securityState = it->second;

    Quantity topBuyQuantity{0};
    Price bestBuy{0};
    if (!securityState.orderBook.getBuyBook().empty())
    {
        const auto &[bestBuyPrice, bestBuyOrders] = *securityState.orderBook.getBuyBook().begin();
        for (const auto &order : bestBuyOrders)
        {
            topBuyQuantity += order.remainingQuantity();
        }
        bestBuy = bestBuyPrice;
    }

    Quantity topSellQuantity{0};
    Price bestSell{0};
    if (!securityState.orderBook.getSellBook().empty())
    {
        const auto &[bestSellPrice, bestSellOrders] = *securityState.orderBook.getSellBook().begin();
        for (const auto &order : bestSellOrders)
        {
            topSellQuantity += order.remainingQuantity();
        }
        bestSell = bestSellPrice;
    }

    return {.bestBuy{bestBuy},
            .bestSell{bestSell},
            .bestBuyQuantity{topBuyQuantity},
            .bestSellQuantity{topSellQuantity},
            .lastTradePrice{securityState.lastTradePrice},
            .lastTradeQuantity{securityState.lastTradeQuantity}};
}

L2Data OrderbookPublisher::getL2Data(const SecurityId securityId, uint8_t numberOfLevels) const
{
    const auto it = state.find(securityId);
    if (it == state.end())
    {
        return {};
    }
    const auto &securityState = it->second;

    numberOfLevels = std::min(L2Data::MAX_LEVELS, numberOfLevels);
    auto buyLevelIt = securityState.orderBook.getBuyBook().begin();
    auto sellLevelIt = securityState.orderBook.getSellBook().begin();
    uint8_t buyLevels{numberOfLevels};
    uint8_t sellLevels{numberOfLevels};
    bool hasMoreBuyLevels = true;
    bool hasMoreSellLevels = true;
    std::array<L2Data::PriceQuantityPair, L2Data::MAX_LEVELS> buyList;
    std::array<L2Data::PriceQuantityPair, L2Data::MAX_LEVELS> sellList;

    for (uint8_t level{0U}; level < numberOfLevels; ++level)
    {
        if (hasMoreBuyLevels)
        {
            if (buyLevelIt == securityState.orderBook.getBuyBook().end())
            {
                buyLevels = level;
                hasMoreBuyLevels = false;
            }
            else
            {
                const auto &[price, orders] = *buyLevelIt;
                Quantity quantity{0};
                for (const auto &order : orders)
                {
                    quantity += order.remainingQuantity();
                }
                buyList[level] = {.price = price, .quantity = quantity};
                ++buyLevelIt;
            }
        }

        if (hasMoreSellLevels)
        {
            if (sellLevelIt == securityState.orderBook.getSellBook().end())
            {
                sellLevels = level;
                hasMoreSellLevels = false;
            }
            else
            {
                const auto &[price, orders] = *sellLevelIt;
                Quantity quantity{0};
                for (const auto &order : orders)
                {
                    quantity += order.remainingQuantity();
                }
                sellList[level] = {.price = price, .quantity = quantity};
                ++sellLevelIt;
            }
        }
    }

    return {.buyList = buyList, .sellList = sellList, .buyLevels = buyLevels, .sellLevels = sellLevels};
}

} // namespace exchange::server