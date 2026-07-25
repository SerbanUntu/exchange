#pragma once
#include "server/common/model/action/action.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server::common::model
{
struct OrderMatchedAction : Action
{
    const Quantity matchedQuantity;
    const bool isFullyMatched;
    const Price price;

    OrderMatchedAction(const OrderId orderId, const AccountId accountId, const Quantity matchedQuantity,
                       const bool isFullyMatched, const Price price)
        : Action(orderId, accountId), matchedQuantity(matchedQuantity), isFullyMatched(isFullyMatched), price(price)
    {
    }
};
} // namespace exchange::server::common::model
