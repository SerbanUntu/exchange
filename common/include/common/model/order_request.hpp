#pragma once
#include <string>

namespace exchange::common::model
{
/**
 * The request for placing an order.
 */
struct OrderRequest
{
    std::string symbol;
    double quantity;
};
} // namespace exchange::common::model
