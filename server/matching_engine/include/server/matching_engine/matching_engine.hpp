#pragma once

#include "server/common/collections/ipc/mpsc_queue.hpp"
#include "server/common/collections/ipc/spmc_queue.hpp"
#include "server/common/collections/orderbook/orderbook.hpp"
#include "server/common/model/action/action.hpp"
#include "server/common/model/event/event.hpp"

#include <span>
#include <unordered_map>
#include <vector>

namespace exchange::server
{
class MatchingEngine
{
    MPSCQueue<Event> &inQueue;
    SPMCQueue<Action> &outQueue;
    std::unordered_map<SecurityId, OrderBook> state;

  public:
    MatchingEngine(MPSCQueue<Event> &inQueue, SPMCQueue<Action> &outQueue, std::span<const SecurityId> securities);
    std::vector<Action> processEvent(const Event &event);
};
} // namespace exchange::server