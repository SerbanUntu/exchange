#pragma once
#include "server/common/model/action/action.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

#include <optional>

namespace exchange::server::common::model
{
struct OrderAmendedAction : Action
{
    const std::optional<Quantity> quantity;
    const std::optional<Price> price;

    OrderAmendedAction(const OrderId orderId, const AccountId accountId, const std::optional<Quantity> quantity,
                         const std::optional<Price> price)
        : Action(orderId, accountId), quantity(quantity), price(price)
    {
    }
};
} // namespace exchange::server::common::model
