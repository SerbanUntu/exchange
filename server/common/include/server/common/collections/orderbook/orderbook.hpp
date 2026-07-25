#pragma once
#include "matched_order.hpp"
#include "order_node.hpp"
#include "price_node.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/price.hpp"

#include <memory>
#include <set>
#include <unordered_map>

namespace exchange::server::common::collections
{
class OrderBook
{
    std::unordered_map<model::Price, std::unique_ptr<PriceNode>> priceLevels;
    std::unordered_map<model::OrderId, std::unique_ptr<OrderNode>> orderMap;
    std::set<model::Price> priceSet;

  public:
    void addOrder(model::OrderId orderId, model::AccountId accountId, model::Price price, model::Quantity quantity,
                  model::TimeInForce timeInForce) noexcept;
    bool removeOrder(model::OrderId orderId, model::AccountId accountId) noexcept;
    bool amendOrder(model::OrderId orderId, model::AccountId accountId, model::Quantity newQuantity) noexcept;
    bool amendOrder(model::OrderId orderId, model::AccountId accountId, model::Price price) noexcept;
    bool amendOrder(model::OrderId orderId, model::AccountId accountId, model::Quantity newQuantity,
                    model::Price price) noexcept;
    std::pair<std::vector<MatchedOrder>, model::Quantity> matchOrdersUp(model::Price minSellPrice,
                                                                        model::Quantity quantity) noexcept;
    std::pair<std::vector<MatchedOrder>, model::Quantity> matchOrdersDown(model::Price maxBuyPrice,
                                                                          model::Quantity quantity) noexcept;
};
} // namespace exchange::server::common::collections