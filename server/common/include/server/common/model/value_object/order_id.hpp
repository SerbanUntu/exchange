#pragma once
#include "server/common/model/value_object/security_id.hpp"
#include <boost/uuid/uuid.hpp>

#include <functional>

namespace exchange::server
{
struct OrderId
{
    boost::uuids::uuid uuid;
    OrderId() = default;
    explicit OrderId(const boost::uuids::uuid uuid) : uuid(uuid)
    {
    }

    bool operator==(const OrderId &other) const
    {
        return uuid == other.uuid;
    }
};
} // namespace exchange::server

template <> struct std::hash<exchange::server::OrderId>
{
    std::size_t operator()(const exchange::server::OrderId &id) const noexcept
    {
        return std::hash<boost::uuids::uuid>{}(id.uuid);
    }
};