#pragma once
#include "server/common/model/value_object/account_id.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server::common::collections
{
struct MatchedOrder
{
    const model::OrderId orderId;
    const model::AccountId accountId;
    const model::Quantity matchedQuantity;
    const bool isFullyMatched;
    const model::Price price;

    MatchedOrder(const model::OrderId orderId, const model::AccountId accountId, const model::Quantity matchedQuantity,
                 const bool isFullyMatched, const model::Price price)
        : orderId(orderId), accountId(accountId), matchedQuantity(matchedQuantity), isFullyMatched(isFullyMatched),
          price(price)
    {
    }
};
} // namespace exchange::server::common::collections