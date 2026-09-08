#pragma once
#include "server/common/model/side.hpp"
#include "server/common/model/action/action.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
struct TradeAction : Action
{
    const OrderId restingId;
    const Price price;
    const Quantity quantity;
    const Side aggressorSide;

    TradeAction(const OrderId aggressorId, const OrderId restingId, const Price price, const Quantity quantity,
                const Side aggressorSide)
        : Action(aggressorId), restingId(restingId), price(price), quantity(quantity), aggressorSide(aggressorSide)
    {
    }

    [[nodiscard]] ActionType getType() const override
    {
        return ActionType::TRADE;
    }
};
} // namespace exchange::server
