#pragma once
#include "server/common/model/event/event.hpp"
#include "server/common/model/side.hpp"
#include "server/common/model/time_in_force.hpp"
#include "server/common/model/value_object/order_id.hpp"
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"
#include "server/common/model/value_object/security_id.hpp"

#include <cstdint>
#include <random>
#include <span>
#include <vector>

namespace exchange::server
{

OrderId makeOrderId(uint8_t seed);

class EventGenerator
{
    struct IssuedOrder
    {
        uint8_t seed;
        SecurityId securityId;
    };

    std::mt19937_64 rng;
    std::vector<SecurityId> securities;
    uint8_t nextOrderSeed = 1;
    std::vector<IssuedOrder> gtcOrders;

    uint64_t below(uint64_t bound);
    Side side();
    Quantity quantity();
    Price price();
    TimeInForce tif();
    SecurityId security();
    IssuedOrder target();

  public:
    EventGenerator(uint64_t seed, std::span<const SecurityId> securities);

    Event next();
};

} // namespace exchange::server
