#pragma once
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

#include <cstdint>
#include <array>

namespace exchange::server
{

struct L2Data
{
    struct PriceQuantityPair
    {
        Price price;
        Quantity quantity;
    };

    static constexpr uint8_t MAX_LEVELS = 10U;
    std::array<PriceQuantityPair, MAX_LEVELS> buyList;  // Descending price
    std::array<PriceQuantityPair, MAX_LEVELS> sellList; // Ascending price
    uint8_t buyLevels;
    uint8_t sellLevels;
};

} // namespace exchange::server