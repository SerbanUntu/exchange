#pragma once
#include "server/common/model/value_object/price.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{

struct L1Data
{
    Price bestBuy;
    Price bestSell;
    Quantity bestBuyQuantity;
    Quantity bestSellQuantity;
    Price lastTradePrice;
    Quantity lastTradeQuantity;
};

} // namespace exchange::server