#include "event_generator.hpp"

#include "server/common/model/event/amend_limit_order_event.hpp"
#include "server/common/model/event/cancel_limit_order_event.hpp"
#include "server/common/model/event/new_limit_order_event.hpp"
#include "server/common/model/event/new_market_order_event.hpp"
#include "server/common/model/value_object/account_id.hpp"

#include <array>

#include <boost/uuid/uuid.hpp>

namespace exchange::server
{
namespace
{
constexpr uint8_t ORDER_ID_POOL = 16;
constexpr uint64_t MIN_PRICE = 98;
constexpr uint64_t PRICE_COUNT = 5;
constexpr uint64_t MAX_QUANTITY = 6;

template <typename Body> Event makeEvent(const OrderId orderId, const SecurityId securityId, const Body &body)
{
    return Event{Event::Header{.orderId = orderId, .securityId = securityId, .accountId = AccountId{}}, body};
}
} // namespace

OrderId makeOrderId(const uint8_t seed)
{
    boost::uuids::uuid uuid{};
    uuid.data[0] = seed;
    return OrderId{uuid};
}

EventGenerator::EventGenerator(const uint64_t seed, const std::span<const SecurityId> securities)
    : rng(seed), securities(securities.begin(), securities.end())
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

Price EventGenerator::price()
{
    return Price{MIN_PRICE + below(PRICE_COUNT)};
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

EventGenerator::IssuedOrder EventGenerator::target()
{
    if (gtcOrders.empty() || below(5) == 0)
        return {.seed = static_cast<uint8_t>(1 + below(ORDER_ID_POOL)), .securityId = security()};
    return gtcOrders.at(below(gtcOrders.size()));
}

Event EventGenerator::next()
{
    auto roll = below(20);
    if (nextOrderSeed > ORDER_ID_POOL && roll < 11)
        roll = 11 + below(9);

    if (roll < 9)
    {
        const auto seed = nextOrderSeed++;
        const auto securityId = security();
        const auto orderTif = tif();
        if (orderTif == TimeInForce::GTC)
            gtcOrders.push_back({.seed = seed, .securityId = securityId});
        return makeEvent(makeOrderId(seed), securityId,
                         NewLimitOrderEvent{.side = side(), .quantity = quantity(), .price = price(), .tif = orderTif});
    }
    if (roll < 11)
    {
        const auto seed = nextOrderSeed++;
        const auto securityId = security();
        return makeEvent(makeOrderId(seed), securityId, NewMarketOrderEvent{.side = side(), .quantity = quantity()});
    }
    const auto [targetSeed, targetSecurityId] = target();
    if (roll < 17)
    {
        const auto variant = below(4);
        return makeEvent(makeOrderId(targetSeed), targetSecurityId,
                         AmendLimitOrderEvent{.quantity = quantity(),
                                              .price = price(),
                                              .isQuantityAmended = variant == 0 || variant == 2,
                                              .isPriceAmended = variant == 1 || variant == 2});
    }
    return makeEvent(makeOrderId(targetSeed), targetSecurityId, CancelLimitOrderEvent{});
}

} // namespace exchange::server
