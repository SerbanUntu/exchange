#pragma once
#include "server/common/collections/orderbook/orderbook.hpp"
#include "server/common/model/action/action.hpp"
#include "server/common/model/event/event.hpp"

#include <vector>

namespace exchange::server::matcher
{

std::vector<std::unique_ptr<common::model::Action>> processEvent(
    const common::model::Event &event,
    std::unordered_map<common::model::SecurityId, common::collections::OrderBook> &state);
/**
 * The starting point of the matching engine.
 *
 * @return The exit code of the program.
 */
int main() noexcept;
} // namespace exchange::server::matcher