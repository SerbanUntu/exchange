#pragma once
#include "server/common/model/action/action.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct OrderMatchedAction : Action
{
    const Quantity matchedQuantity;
    const Price price;

    OrderMatchedAction(const OrderId orderId, const AccountId accountId, const Quantity matchedQuantity,
                       const Price price)
        : Action(orderId, accountId), matchedQuantity(matchedQuantity), price(price)
    {
    }

    [[nodiscard]] ActionType getType() const override
    {
        return ActionType::ORDER_MATCHED;
    }
};
} // namespace exchange::server
