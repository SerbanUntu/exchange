#pragma once

#include "server/common/collections/orderbook/orderbook.hpp"
#include "server/common/model/action/action.hpp"
#include "server/common/model/event/event.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace exchange::server
{
std::vector<std::unique_ptr<Action>> processEvent(const Event &event, std::unordered_map<SecurityId, OrderBook> &state);
} // namespace exchange::server