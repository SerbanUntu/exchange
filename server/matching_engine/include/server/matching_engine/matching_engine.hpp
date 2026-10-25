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

    /**
     * Perform one iteration of the matching engine's workflow.
     *
     * Pops from and pushes to the shared memory queues.
     */
    void loop();

    /**
     * Given an event from the gateway requesting that something happen to an order,
     * potentially act on that request to mutate the orderbook and obtain a list of changes to it,
     * for downstream consumers.
     *
     * @param event The event coming from the gateway.
     * @return The list of actions that were performed on the internal orderbook.
     */
    std::vector<Action> processEvent(const Event &event);
};
} // namespace exchange::server