#pragma once
#include "server/common/model/time_in_force.hpp"
#include "server/common/model/value_object/account_id.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server::common::collections
{
class OrderNode
{
    const model::OrderId orderId;
    const model::AccountId accountId;
    model::Quantity totalQuantity;
    model::Quantity executedQuantity{0};
    const model::TimeInForce timeInForce;
    OrderNode *prev{nullptr}; // non-owning
    OrderNode *next{nullptr}; // non-owning

  public:
    explicit OrderNode(const model::OrderId orderId, const model::AccountId accountId, const model::Quantity totalQuantity,
                       const model::TimeInForce timeInForce)
        : orderId(orderId), accountId(accountId), totalQuantity(totalQuantity), timeInForce(timeInForce)
    {
    }

    [[nodiscard]] model::OrderId getOrderId() const
    {
        return orderId;
    }

    [[nodiscard]] model::AccountId getAccountId() const
    {
        return accountId;
    }

    [[nodiscard]] model::Quantity getTotalQuantity() const
    {
        return totalQuantity;
    }

    void setTotalQuantity(const model::Quantity newTotalQuantity)
    {
        totalQuantity = newTotalQuantity;
    }

    [[nodiscard]] model::Quantity getExecutedQuantity() const
    {
        return executedQuantity;
    }

    void setExecutedQuantity(const model::Quantity newExecutedQuantity)
    {
        executedQuantity = newExecutedQuantity;
    }

    [[nodiscard]] model::TimeInForce getTimeInForce() const
    {
        return timeInForce;
    }

    [[nodiscard]] OrderNode *getPrev()
    {
        return prev;
    }

    [[nodiscard]] const OrderNode *getPrev() const
    {
        return prev;
    }

    void setPrev(OrderNode *newPrev)
    {
        prev = newPrev;
    }

    [[nodiscard]] OrderNode *getNext()
    {
        return next;
    }

    [[nodiscard]] const OrderNode *getNext() const
    {
        return next;
    }

    void setNext(OrderNode *newNext)
    {
        next = newNext;
    }
};
}; // namespace exchange::server::common::collections
