#include "server/matching_engine/fuzzing/event_encoding.hpp"

#include "server/common/model/event/amend_limit_order_event.hpp"
#include "server/common/model/event/cancel_limit_order_event.hpp"
#include "server/common/model/event/new_limit_order_event.hpp"
#include "server/common/model/event/new_market_order_event.hpp"

#include <boost/uuid/uuid.hpp>

#include <cstdint>
#include <cstdio>
#include <exception>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
using namespace exchange::server;

constexpr SecurityId SECURITY_ID = FUZZING_SECURITY_IDS[0];

template <typename Body> Event makeEvent(const uint8_t seed, const Body &body)
{
    boost::uuids::uuid uuid{};
    uuid.data[0] = seed;
    return Event{.header = Event::Header{.orderId = OrderId{uuid}, .securityId = SECURITY_ID, .accountId = AccountId{}},
                 .body = body};
}

Event limit(const uint8_t seed, const Side side, const uint64_t price, const uint64_t quantity,
            const TimeInForce tif = TimeInForce::GTC)
{
    return makeEvent(
        seed, NewLimitOrderEvent{.side = side, .quantity = Quantity{quantity}, .price = Price{price}, .tif = tif});
}

Event market(const uint8_t seed, const Side side, const uint64_t quantity)
{
    return makeEvent(seed, NewMarketOrderEvent{.side = side, .quantity = Quantity{quantity}});
}

Event amendQuantity(const uint8_t seed, const uint64_t quantity)
{
    return makeEvent(seed, AmendLimitOrderEvent{.quantity = Quantity{quantity}, .isQuantityAmended = true});
}

Event amendPrice(const uint8_t seed, const uint64_t price)
{
    return makeEvent(seed, AmendLimitOrderEvent{.price = Price{price}, .isPriceAmended = true});
}

Event cancel(const uint8_t seed)
{
    return makeEvent(seed, CancelLimitOrderEvent{});
}

void writeSeeds()
{
    const std::vector fokReject{limit(1, Side::SELL, 100, 2), limit(2, Side::BUY, 100, 5, TimeInForce::FOK)};
    const std::vector fokFillAcrossLevels{limit(1, Side::SELL, 100, 2), limit(2, Side::SELL, 101, 2),
                                          limit(3, Side::BUY, 101, 4, TimeInForce::FOK)};
    const std::vector marketSweepLevels{limit(1, Side::SELL, 100, 2), limit(2, Side::SELL, 101, 2),
                                        market(3, Side::BUY, 3)};
    const std::vector iocNotKept{limit(1, Side::SELL, 100, 2), limit(2, Side::BUY, 100, 5, TimeInForce::IOC), cancel(2),
                                 limit(3, Side::SELL, 100, 3)};
    const std::vector gtcRemainderRests{limit(1, Side::SELL, 100, 3), limit(2, Side::BUY, 100, 10)};
    const std::vector upsizeLosesPriority{limit(1, Side::SELL, 100, 2), limit(2, Side::SELL, 100, 2),
                                          amendQuantity(1, 5), limit(3, Side::BUY, 100, 7)};
    const std::vector amendDownToFilled{limit(1, Side::SELL, 100, 10), limit(2, Side::BUY, 100, 4),
                                        amendQuantity(1, 4)};
    const std::vector repricePartiallyFilled{limit(1, Side::BUY, 100, 10), limit(2, Side::SELL, 100, 4),
                                             amendPrice(1, 99), amendQuantity(1, 8)};
    const std::vector amendPriceCross{limit(1, Side::SELL, 105, 5), limit(2, Side::BUY, 100, 5), amendPrice(2, 105)};
    const std::vector rejectedAmend{amendQuantity(1, 5), limit(2, Side::SELL, 100, 10), amendQuantity(2, 10)};

    const std::unordered_map<std::string, std::span<const Event>> eventsMap = {
        {"fok_reject", fokReject},
        {"fok_fill_across_levels", fokFillAcrossLevels},
        {"market_sweep_levels", marketSweepLevels},
        {"ioc_not_kept", iocNotKept},
        {"gtc_remainder_rests", gtcRemainderRests},
        {"upsize_loses_priority", upsizeLosesPriority},
        {"amend_down_to_filled", amendDownToFilled},
        {"reprice_partially_filled", repricePartiallyFilled},
        {"amend_price_cross", amendPriceCross},
        {"rejected_amend", rejectedAmend},
    };
    serializeEventsIntoSeedsDirectory(eventsMap);
}
} // namespace

int main()
{
    try
    {
        writeSeeds();
        return 0;
    }
    catch (const std::exception &err)
    {
        std::fprintf(stderr, "Failed to write the seeds: %s\n", err.what());
        return 1;
    }
}
