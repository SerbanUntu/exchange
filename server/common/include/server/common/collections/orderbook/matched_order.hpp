#pragma once
#include "server/common/model/value_object/account_id.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct MatchedOrder
{
    const OrderId orderId;
    const AccountId accountId;
    const Quantity matchedQuantity;
    const bool isFullyMatched;
    const Price price;

    MatchedOrder(OrderId orderId, AccountId accountId, Quantity matchedQuantity, const bool isFullyMatched,
                 const Price price)
        : orderId(orderId), accountId(accountId), matchedQuantity(matchedQuantity), isFullyMatched(isFullyMatched),
          price(price)
    {
    }
};
} // namespace exchange::server