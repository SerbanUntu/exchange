#pragma once
#include "server/common/model/action/action.hpp"
#include "server/common/model/event/event.hpp"
#include "server/common/model/side.hpp"
#include "server/common/model/time_in_force.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"
#include "server/common/model/value_object/security_id.hpp"

#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <unordered_map>
#include <vector>

namespace exchange::server
{

OrderId makeOrderId(uint64_t seed);

class EventGenerator
{
  public:
    struct Config
    {
        uint64_t orderIdPool;
        uint64_t minPrice;
        uint64_t priceCount;
        uint64_t sidePriceOffset;
        uint64_t newLimitWeight;
        uint64_t marketWeight;
        uint64_t amendWeight;
        uint64_t cancelWeightPerLiveOrder;
    };

  private:
    struct LiveOrder
    {
        OrderId orderId;
        SecurityId securityId;
        Side side;
    };

    std::mt19937_64 rng;
    std::vector<SecurityId> securities;
    Config config;
    uint64_t nextOrderSeed = 1;
    std::vector<LiveOrder> liveOrders;
    std::unordered_map<OrderId, std::size_t> liveOrderIndices;

    uint64_t below(uint64_t bound);
    Side side();
    Quantity quantity();
    Price price(Side orderSide);
    TimeInForce tif();
    SecurityId security();
    LiveOrder target();
    void addLiveOrder(const LiveOrder &order);
    void removeLiveOrder(OrderId orderId);

  public:
    EventGenerator(uint64_t seed, std::span<const SecurityId> securities, Config config);

    Event next();

    void observeActions(std::span<const Action> actions);
};

inline constexpr EventGenerator::Config EVENT_GENERATOR_CONFIG_FOR_DIFFERENTIAL_TESTING{
    .orderIdPool = 16,
    .minPrice = 98,
    .priceCount = 5,
    .sidePriceOffset = 0,
    .newLimitWeight = 90,
    .marketWeight = 20,
    .amendWeight = 60,
    .cancelWeightPerLiveOrder = 6,
};

} // namespace exchange::server
