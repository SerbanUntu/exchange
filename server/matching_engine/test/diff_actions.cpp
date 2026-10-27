#include "diff_actions.hpp"

#include "common/util/overloads.hpp"
#include "server/common/model/action/order_added_action.hpp"
#include "server/common/model/action/order_downsized_action.hpp"
#include "server/common/model/action/order_removed_action.hpp"
#include "server/common/model/action/order_upsized_action.hpp"
#include "server/common/model/action/trade_action.hpp"
#include "server/common/model/event/amend_limit_order_event.hpp"
#include "server/common/model/event/cancel_limit_order_event.hpp"
#include "server/common/model/event/new_limit_order_event.hpp"
#include "server/common/model/event/new_market_order_event.hpp"
#include "server/common/model/side.hpp"
#include "server/common/model/time_in_force.hpp"
#include "server/common/model/value_object/order_id.hpp"

#include <algorithm>
#include <cstddef>
#include <variant>

#include <boost/uuid/uuid_io.hpp>

namespace exchange::server
{
namespace
{
std::string toString(const OrderId &orderId)
{
    if (std::all_of(orderId.uuid.begin() + 1, orderId.uuid.end(), [](const auto byte) { return byte == 0; }))
        return "o" + std::to_string(orderId.uuid.data[0]);
    return boost::uuids::to_string(orderId.uuid);
}

std::string toString(const Side side)
{
    return side == Side::BUY ? "BUY" : "SELL";
}

std::string toString(const TimeInForce tif)
{
    switch (tif)
    {
    case TimeInForce::GTC:
        return "GTC";
    case TimeInForce::IOC:
        return "IOC";
    case TimeInForce::FOK:
        return "FOK";
    }
    return "?";
}
} // namespace

std::string toString(const Action &action)
{
    const auto prefix = "sec" + std::to_string(action.header.securityId.value) + " " + toString(action.header.orderId);
    return prefix + " " +
           std::visit(overloads{
                          [](const OrderAddedAction &body) {
                              return "ADDED " + toString(body.side) + " " + std::to_string(body.totalQuantity.value) +
                                     "@" + std::to_string(body.price.value) +
                                     " filled=" + std::to_string(body.filledQuantity.value);
                          },
                          [](const OrderDownsizedAction &body) {
                              return "DOWNSIZED to " + std::to_string(body.newQuantity.value);
                          },
                          [](const OrderRemovedAction &) { return std::string{"REMOVED"}; },
                          [](const OrderUpsizedAction &body) {
                              return "UPSIZED to " + std::to_string(body.newQuantity.value);
                          },
                          [](const TradeAction &body) {
                              return "TRADE " + toString(body.aggressorSide) + " " +
                                     std::to_string(body.quantity.value) + "@" + std::to_string(body.price.value) +
                                     " against " + toString(body.restingId) +
                                     " restingLeft=" + std::to_string(body.remainingRestingQuantity.value);
                          },
                      },
                      action.body);
}

std::string toString(const Event &event)
{
    const auto prefix = "sec" + std::to_string(event.header.securityId.value) + " " + toString(event.header.orderId);
    return prefix + " " +
           std::visit(overloads{
                          [](const NewLimitOrderEvent &body) {
                              return "LIMIT " + toString(body.side) + " " + std::to_string(body.quantity.value) + "@" +
                                     std::to_string(body.price.value) + " " + toString(body.tif);
                          },
                          [](const NewMarketOrderEvent &body) {
                              return "MARKET " + toString(body.side) + " " + std::to_string(body.quantity.value);
                          },
                          [](const AmendLimitOrderEvent &body) {
                              std::string result = "AMEND";
                              if (body.isQuantityAmended)
                                  result += " quantity=" + std::to_string(body.quantity.value);
                              if (body.isPriceAmended)
                                  result += " price=" + std::to_string(body.price.value);
                              if (!body.isQuantityAmended && !body.isPriceAmended)
                                  result += " nothing";
                              return result;
                          },
                          [](const CancelLimitOrderEvent &) { return std::string{"CANCEL"}; },
                      },
                      event.body);
}

std::string diffActions(const std::vector<Action> &expected, const std::vector<Action> &actual)
{
    std::string diff;
    for (std::size_t i = 0; i < std::max(expected.size(), actual.size()); ++i)
    {
        const auto expectedLine = i < expected.size() ? toString(expected[i]) : "<none>";
        const auto actualLine = i < actual.size() ? toString(actual[i]) : "<none>";
        if (expectedLine != actualLine)
        {
            const auto index = "  [" + std::to_string(i) + "] ";
            diff += index;
            diff += "expected: ";
            diff += expectedLine;
            diff += "\n";
            diff += index;
            diff += "actual:   ";
            diff += actualLine;
            diff += "\n";
        }
    }
    return diff;
}

} // namespace exchange::server
