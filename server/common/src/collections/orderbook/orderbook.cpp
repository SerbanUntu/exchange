#include "server/common/collections/orderbook/orderbook.hpp"

namespace exchange::server::common::collections
{
void OrderBook::addOrder(model::OrderId orderId, model::AccountId accountId, const model::Side side, model::Price price,
                         model::Quantity quantity, model::TimeInForce timeInForce) noexcept
{
    auto ownedOrderNode{std::make_unique<OrderNode>(orderId, accountId, quantity, timeInForce)};
    auto *const orderNode{ownedOrderNode.get()};
    PriceNode *priceNode{nullptr};
    auto &priceLevels{side == model::Side::BUY ? buyLevels : sellLevels};
    auto &priceSet{side == model::Side::BUY ? buyPriceSet : sellPriceSet};

    if (const auto it{priceLevels.find(price)}; it == priceLevels.end())
    {
        auto ownedPriceNode{std::make_unique<PriceNode>(price)};
        priceNode = ownedPriceNode.get();
        const auto high{priceSet.lower_bound(price)};
        const auto low{priceSet.upper_bound(price)};
        if (high != priceSet.end())
        {
            auto *highNode{priceLevels[*high].get()};
            highNode->setPrev(priceNode);
            priceNode->setNext(highNode);
        }
        if (low != priceSet.end())
        {
            auto *lowNode{priceLevels[*low].get()};
            lowNode->setNext(priceNode);
            priceNode->setPrev(lowNode);
        }
        priceSet.insert(price);
        priceLevels[price] = std::move(ownedPriceNode);
    }
    else
    {
        priceNode = priceLevels[price].get();
    }
    auto *tail{priceNode->getTail()};
    tail->setNext(orderNode);
    orderNode->setPrev(tail);
    priceNode->setTail(orderNode);
    orderMap[orderId] = std::move(ownedOrderNode);
}
} // namespace exchange::server::common::collections