#include "server/matching_engine/fuzzing/event_encoding.hpp"

#include "common/util/overloads.hpp"
#include "server/common/model/event/amend_limit_order_event.hpp"
#include "server/common/model/event/cancel_limit_order_event.hpp"
#include "server/common/model/event/new_limit_order_event.hpp"
#include "server/common/model/event/new_market_order_event.hpp"
#include "server/common/model/side.hpp"
#include "server/common/model/time_in_force.hpp"
#include "server/common/model/value_object/account_id.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

#include <boost/uuid/uuid.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>

namespace exchange::server
{
namespace
{
constexpr std::size_t RECORD_SIZE = 6;
constexpr uint8_t ORDER_ID_POOL = 32;
constexpr uint64_t MIN_PRICE = 95;
constexpr uint64_t PRICE_COUNT = 16;
constexpr uint64_t MIN_QUANTITY = 1;
constexpr uint64_t QUANTITY_COUNT = 16;
constexpr std::array TIME_IN_FORCES{TimeInForce::GTC, TimeInForce::IOC, TimeInForce::FOK, TimeInForce::GTC};

enum class EventKind : uint8_t
{
    NEW_LIMIT,
    NEW_MARKET,
    AMEND,
    CANCEL
};

constexpr uint8_t SIDE_BIT = 0b001;
constexpr uint8_t TIME_IN_FORCE_SHIFT = 1;
constexpr uint8_t QUANTITY_AMENDED_BIT = 0b001;
constexpr uint8_t PRICE_AMENDED_BIT = 0b010;

OrderId orderIdFromSeed(const uint8_t seed)
{
    boost::uuids::uuid uuid{};
    uuid.data[0] = seed;
    return OrderId{uuid};
}

uint8_t encodeInRange(const uint64_t value, const uint64_t min, const uint64_t count)
{
    if (value < min || value >= min + count)
    {
        throw std::out_of_range("Cannot encode " + std::to_string(value) + ", expected a value in [" +
                                std::to_string(min) + ", " + std::to_string(min + count) + ")");
    }
    return static_cast<uint8_t>(value - min);
}

uint8_t encodeOrderId(const OrderId orderId)
{
    const auto seed = orderId.uuid.data[0];
    if (orderIdFromSeed(seed) != orderId)
    {
        throw std::out_of_range("Cannot encode an order id that was not created from a single byte seed");
    }
    return encodeInRange(seed, 1, ORDER_ID_POOL);
}

std::optional<uint8_t> claimOrderIdSeed(std::array<bool, ORDER_ID_POOL + 1> &issued, const uint8_t requested)
{
    if (!issued.at(requested))
    {
        issued.at(requested) = true;
        return requested;
    }
    for (uint8_t seed = 1; seed <= ORDER_ID_POOL; ++seed)
    {
        if (!issued.at(seed))
        {
            issued.at(seed) = true;
            return seed;
        }
    }
    return std::nullopt;
}
} // namespace

std::vector<Event> decodeEvents(const std::span<const uint8_t> bytes, const std::span<const SecurityId> securities)
{
    std::vector<Event> events;
    std::array<bool, ORDER_ID_POOL + 1> issued{};
    for (std::size_t offset = 0; offset + RECORD_SIZE <= bytes.size(); offset += RECORD_SIZE)
    {
        const auto record = bytes.subspan(offset, RECORD_SIZE);
        const auto kind = static_cast<EventKind>(record[0] % 4);
        const auto securityId = securities[record[1] % securities.size()];
        auto seed = static_cast<uint8_t>(1 + (record[2] % ORDER_ID_POOL));
        const auto flags = record[3];
        const Price price{MIN_PRICE + (record[4] % PRICE_COUNT)};
        const Quantity quantity{MIN_QUANTITY + (record[5] % QUANTITY_COUNT)};
        const auto side = static_cast<Side>(flags & SIDE_BIT);

        if (kind == EventKind::NEW_LIMIT || kind == EventKind::NEW_MARKET)
        {
            const auto claimed = claimOrderIdSeed(issued, seed);
            if (!claimed.has_value())
                continue;
            seed = *claimed;
        }

        const Event::Header header{
            .orderId = orderIdFromSeed(seed), .securityId = securityId, .accountId = AccountId{}};
        switch (kind)
        {
        case EventKind::NEW_LIMIT:
            events.push_back(
                Event{.header = header,
                      .body = NewLimitOrderEvent{.side = side,
                                                 .quantity = quantity,
                                                 .price = price,
                                                 .tif = TIME_IN_FORCES.at((flags >> TIME_IN_FORCE_SHIFT) & 0b11)}});
            break;
        case EventKind::NEW_MARKET:
            events.push_back(Event{.header = header, .body = NewMarketOrderEvent{.side = side, .quantity = quantity}});
            break;
        case EventKind::AMEND: {
            const bool isQuantityAmended = (flags & QUANTITY_AMENDED_BIT) != 0;
            const bool isPriceAmended = (flags & PRICE_AMENDED_BIT) != 0;
            events.push_back(Event{.header = header,
                                   .body = AmendLimitOrderEvent{.quantity = isQuantityAmended ? quantity : Quantity{},
                                                                .price = isPriceAmended ? price : Price{},
                                                                .isQuantityAmended = isQuantityAmended,
                                                                .isPriceAmended = isPriceAmended}});
            break;
        }
        case EventKind::CANCEL:
            events.push_back(Event{.header = header, .body = CancelLimitOrderEvent{}});
            break;
        }
    }
    return events;
}

std::vector<uint8_t> encodeEvents(const std::span<const Event> events, const std::span<const SecurityId> securities)
{
    std::vector<uint8_t> bytes;
    bytes.reserve(events.size() * RECORD_SIZE);
    for (const auto &event : events)
    {
        const auto securityIt = std::ranges::find(securities, event.header.securityId);
        if (securityIt == securities.end())
        {
            throw std::out_of_range("Cannot encode unknown security id " +
                                    std::to_string(event.header.securityId.value));
        }

        std::array<uint8_t, RECORD_SIZE> record{};
        record[1] = static_cast<uint8_t>(securityIt - securities.begin());
        record[2] = encodeOrderId(event.header.orderId);
        std::visit(
            overloads{
                [&record](const NewLimitOrderEvent &body) {
                    record[0] = static_cast<uint8_t>(EventKind::NEW_LIMIT);
                    record[3] = static_cast<uint8_t>(static_cast<uint8_t>(body.side) | static_cast<uint8_t>(body.tif)
                                                                                           << TIME_IN_FORCE_SHIFT);
                    record[4] = encodeInRange(body.price.value, MIN_PRICE, PRICE_COUNT);
                    record[5] = encodeInRange(body.quantity.value, MIN_QUANTITY, QUANTITY_COUNT);
                },
                [&record](const NewMarketOrderEvent &body) {
                    record[0] = static_cast<uint8_t>(EventKind::NEW_MARKET);
                    record[3] = static_cast<uint8_t>(body.side);
                    record[5] = encodeInRange(body.quantity.value, MIN_QUANTITY, QUANTITY_COUNT);
                },
                [&record](const AmendLimitOrderEvent &body) {
                    record[0] = static_cast<uint8_t>(EventKind::AMEND);
                    record[3] = static_cast<uint8_t>((body.isQuantityAmended ? QUANTITY_AMENDED_BIT : 0) |
                                                     (body.isPriceAmended ? PRICE_AMENDED_BIT : 0));
                    if (body.isPriceAmended)
                        record[4] = encodeInRange(body.price.value, MIN_PRICE, PRICE_COUNT);
                    if (body.isQuantityAmended)
                        record[5] = encodeInRange(body.quantity.value, MIN_QUANTITY, QUANTITY_COUNT);
                },
                [&record](const CancelLimitOrderEvent &) { record[0] = static_cast<uint8_t>(EventKind::CANCEL); },
            },
            event.body);
        bytes.insert(bytes.end(), record.begin(), record.end());
    }
    return bytes;
}

void serializeEventsIntoSeedsDirectory(const std::unordered_map<std::string, std::span<const Event>> &seeds)
{
    const std::filesystem::path directory{SEEDS_DIRECTORY};
    std::filesystem::create_directories(directory);
    for (const auto &[name, events] : seeds)
    {
        const auto bytes = encodeEvents(events, FUZZING_SECURITY_IDS);
        std::ofstream file(directory / name, std::ios::binary | std::ios::trunc);
        file.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
}

} // namespace exchange::server
