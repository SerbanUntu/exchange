#pragma once
#include "server/common/model/value_object/security_id.hpp"
#include <boost/uuid/uuid.hpp>

#include <functional>

namespace exchange::server::common::model
{
struct OrderId
{
    boost::uuids::uuid uuid;
    SecurityId securityId;
    OrderId(const boost::uuids::uuid uuid, const SecurityId securityId) : uuid(uuid), securityId(securityId)
    {
    }

    bool operator==(const OrderId &other) const
    {
        return uuid == other.uuid && securityId == other.securityId;
    }
};
} // namespace exchange::server::common::model

template <> struct std::hash<exchange::server::common::model::OrderId>
{
    std::size_t operator()(const exchange::server::common::model::OrderId &id) const noexcept
    {
        return std::hash<boost::uuids::uuid>{}(id.uuid);
    }
};