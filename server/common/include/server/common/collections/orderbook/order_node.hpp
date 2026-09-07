#pragma once
#include "server/common/model/time_in_force.hpp"
#include "server/common/model/value_object/account_id.hpp"
#include "server/common/model/value_object/quantity.hpp"

namespace exchange::server
{
class OrderNode
{
    const OrderId orderId;
    const AccountId accountId;
    Quantity totalQuantity;
    Quantity executedQuantity{0};
    const TimeInForce timeInForce;
    OrderNode *prev{nullptr};
    OrderNode *next{nullptr};

  public:
    explicit OrderNode(const OrderId orderId, const AccountId accountId, const Quantity totalQuantity,
                       const TimeInForce timeInForce)
        : orderId(orderId), accountId(accountId), totalQuantity(totalQuantity), timeInForce(timeInForce)
    {
    }

    [[nodiscard]] OrderId getOrderId() const
    {
        return orderId;
    }

    [[nodiscard]] AccountId getAccountId() const
    {
        return accountId;
    }

    [[nodiscard]] Quantity getTotalQuantity() const
    {
        return totalQuantity;
    }

    void setTotalQuantity(const Quantity newTotalQuantity)
    {
        totalQuantity = newTotalQuantity;
    }

    [[nodiscard]] Quantity getExecutedQuantity() const
    {
        return executedQuantity;
    }

    void setExecutedQuantity(const Quantity newExecutedQuantity)
    {
        executedQuantity = newExecutedQuantity;
    }

    [[nodiscard]] TimeInForce getTimeInForce() const
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
}; // namespace exchange::server
