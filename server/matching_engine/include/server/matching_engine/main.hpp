#pragma once
#include "server/common/collections/orderbook/orderbook.hpp"
#include "server/common/model/action/action.hpp"
#include "server/common/model/event/event.hpp"
#include "server/common/model/value_object/security_id.hpp"

#include <vector>

namespace exchange::server
{

std::vector<std::unique_ptr<Action>> processEvent(
    const Event &event,
    std::unordered_map<SecurityId, OrderBook> &state);
/**
 * The starting point of the matching engine.
 *
 * @return The exit code of the program.
 */
int matchingEngineMain() noexcept;
} // namespace exchange::server