#include "server/matching_engine/event_generator.hpp"

#include "server/common/model/action/order_added_action.hpp"
#include "server/common/model/action/order_removed_action.hpp"
#include "server/common/model/event/amend_limit_order_event.hpp"
#include "server/common/model/event/cancel_limit_order_event.hpp"
#include "server/common/model/event/new_limit_order_event.hpp"
#include "server/common/model/event/new_market_order_event.hpp"
#include "server/common/model/value_object/account_id.hpp"

#include <array>
#include <variant>

#include <boost/uuid/uuid.hpp>

namespace exchange::server
{
namespace
{
constexpr uint64_t MAX_QUANTITY = 6;
constexpr std::size_t BITS_PER_BYTE = 8;

template <typename Body> Event makeEvent(const OrderId orderId, const SecurityId securityId, const Body &body)
{
    return Event{Event::Header{.orderId = orderId, .securityId = securityId, .accountId = AccountId{}}, body};
}
} // namespace

OrderId makeOrderId(const uint64_t seed)
{
    boost::uuids::uuid uuid{};
    for (std::size_t byte = 0; byte < sizeof(seed); ++byte)
        uuid.data[byte] = static_cast<uint8_t>(seed >> (byte * BITS_PER_BYTE));
    return OrderId{uuid};
}

EventGenerator::EventGenerator(const uint64_t seed, const std::span<const SecurityId> securities, const Config config)
    : rng(seed), securities(securities.begin(), securities.end()), config(config)
{
}

uint64_t EventGenerator::below(const uint64_t bound)
{
    return rng() % bound;
}

Side EventGenerator::side()
{
    return below(2) == 0 ? Side::BUY : Side::SELL;
}

Quantity EventGenerator::quantity()
{
    return Quantity{1 + below(MAX_QUANTITY)};
}

Price EventGenerator::price(const Side orderSide)
{
    const auto level = config.minPrice + below(config.priceCount);
    return Price{orderSide == Side::BUY ? level - config.sidePriceOffset : level + config.sidePriceOffset};
}

TimeInForce EventGenerator::tif()
{
    constexpr std::array tifs{TimeInForce::GTC, TimeInForce::GTC, TimeInForce::IOC, TimeInForce::FOK};
    return tifs.at(below(tifs.size()));
}

SecurityId EventGenerator::security()
{
    return securities.at(below(securities.size()));
}

EventGenerator::LiveOrder EventGenerator::target()
{
    if (liveOrders.empty() || below(5) == 0)
        return {.orderId = makeOrderId(1 + below(config.orderIdPool)), .securityId = security(), .side = Side::BUY};
    return liveOrders.at(below(liveOrders.size()));
}

void EventGenerator::addLiveOrder(const LiveOrder &order)
{
    if (liveOrderIndices.try_emplace(order.orderId, liveOrders.size()).second)
        liveOrders.push_back(order);
}

void EventGenerator::removeLiveOrder(const OrderId orderId)
{
    const auto entry = liveOrderIndices.find(orderId);
    if (entry == liveOrderIndices.end())
        return;
    const auto index = entry->second;
    liveOrderIndices.erase(entry);
    if (index != liveOrders.size() - 1)
    {
        liveOrders[index] = liveOrders.back();
        liveOrderIndices[liveOrders[index].orderId] = index;
    }
    liveOrders.pop_back();
}

Event EventGenerator::next()
{
    const auto newOrderWeight = config.newLimitWeight + config.marketWeight;
    const auto existingOrderWeight = config.amendWeight + (config.cancelWeightPerLiveOrder * liveOrders.size());
    auto roll = below(newOrderWeight + existingOrderWeight);
    if (nextOrderSeed > config.orderIdPool && roll < newOrderWeight)
        roll = newOrderWeight + below(existingOrderWeight);

    if (roll < config.newLimitWeight)
    {
        const auto orderId = makeOrderId(nextOrderSeed++);
        const auto securityId = security();
        const auto orderTif = tif();
        const auto orderSide = side();
        return makeEvent(
            orderId, securityId,
            NewLimitOrderEvent{.side = orderSide, .quantity = quantity(), .price = price(orderSide), .tif = orderTif});
    }
    if (roll < newOrderWeight)
    {
        const auto orderId = makeOrderId(nextOrderSeed++);
        const auto securityId = security();
        return makeEvent(orderId, securityId, NewMarketOrderEvent{.side = side(), .quantity = quantity()});
    }
    const auto [targetId, targetSecurityId, targetSide] = target();
    if (roll < newOrderWeight + config.amendWeight)
    {
        const auto variant = below(4);
        return makeEvent(targetId, targetSecurityId,
                         AmendLimitOrderEvent{.quantity = quantity(),
                                              .price = price(targetSide),
                                              .isQuantityAmended = variant == 0 || variant == 2,
                                              .isPriceAmended = variant == 1 || variant == 2});
    }
    return makeEvent(targetId, targetSecurityId, CancelLimitOrderEvent{});
}

void EventGenerator::observeActions(const std::span<const Action> actions)
{
    for (const auto &action : actions)
    {
        if (const auto *added = std::get_if<OrderAddedAction>(&action.body))
            addLiveOrder(
                {.orderId = action.header.orderId, .securityId = action.header.securityId, .side = added->side});
        else if (std::holds_alternative<OrderRemovedAction>(action.body))
            removeLiveOrder(action.header.orderId);
    }
}

} // namespace exchange::server
