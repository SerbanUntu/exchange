#pragma once
#include "server/common/model/value_object/price.hpp"

namespace exchange::server
{
class PriceNode
{
    const Price price;
    PriceNode *prev{nullptr}; // non-owning
    PriceNode *next{nullptr}; // non-owning
    OrderNode *head{nullptr}; // non-owning
    OrderNode *tail{nullptr}; // non-owning

  public:
    explicit PriceNode(const Price price) : price(price)
    {
    }

    [[nodiscard]] Price getPrice() const
    {
        return price;
    }

    [[nodiscard]] PriceNode *getPrev()
    {
        return prev;
    }

    [[nodiscard]] const PriceNode *getPrev() const
    {
        return prev;
    }

    void setPrev(PriceNode *newPrev)
    {
        prev = newPrev;
    }

    [[nodiscard]] PriceNode *getNext()
    {
        return next;
    }

    [[nodiscard]] const PriceNode *getNext() const
    {
        return next;
    }

    void setNext(PriceNode *newNext)
    {
        next = newNext;
    }

    [[nodiscard]] OrderNode *getHead()
    {
        return head;
    }

    [[nodiscard]] const OrderNode *getHead() const
    {
        return head;
    }

    void setHead(OrderNode *newHead)
    {
        head = newHead;
    }

    [[nodiscard]] OrderNode *getTail()
    {
        return tail;
    }

    [[nodiscard]] const OrderNode *getTail() const
    {
        return tail;
    }

    void setTail(OrderNode *newTail)
    {
        tail = newTail;
    }
};

} // namespace exchange::server