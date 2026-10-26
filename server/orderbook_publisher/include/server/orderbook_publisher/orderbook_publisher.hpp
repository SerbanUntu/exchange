#pragma once
#include <cstdint>

#include "server/common/collections/ipc/action_spmc_receiver.hpp"
#include "server/common/collections/orderbook/l1_data.hpp"
#include "server/common/collections/orderbook/l2_data.hpp"
#include "server/common/collections/orderbook/orderbook.hpp"

namespace exchange::server
{

class OrderbookPublisher
{
    struct SecurityState
    {
        OrderBook orderBook;
        Price lastTradePrice;
        Quantity lastTradeQuantity;
    };

    ActionSpmcReceiver actionReceiver;
    std::unordered_map<SecurityId, SecurityState> state;

  public:
    explicit OrderbookPublisher(ActionSpmcReceiver &&actionReceiver);

    /**
     * Perform one iteration of the orderbook publisher's workflow, which
     * consumes one action from the SPMC queue and updates its internal state.
     */
    void loop();

    /**
     * Update the internal orderbook of the publisher based on an update from the matching engine.
     *
     * @param action The modification to the orderbook state, coming from the SPMC queue.
     */
    void processAction(const Action &action);

    /**
     * Compute the L1 data for a given security.
     *
     * @param securityId The security for which to compute the L1 data.
     * @return The L1 data of that security's orderbook, consisting of best buy/sell price and quantity,
     * as well as the price and quantity of the most recent trade.
     */
    [[nodiscard]] L1Data getL1Data(SecurityId securityId) const;

    /**
     * Compute the L2 data for a given security, with a given number of levels.
     *
     * @param securityId The security for which to compute the L2 data.
     * @param numberOfLevels The number of levels (price, quantity pairs) to compute for each of the buy and sell books.
     * At most 10 (guaranteed by the gateway).
     * @return The L2 data, which contains the price and quantity pairs for the best `numberOfLevels` levels.
     */
    [[nodiscard]] L2Data getL2Data(SecurityId securityId, uint8_t numberOfLevels) const;
};

} // namespace exchange::server