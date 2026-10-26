#include "server/gateway/gateway.hpp"

#include "common/model/universe.hpp"

#include <algorithm>

#include <grpcpp/grpcpp.h>

namespace exchange::server
{

Gateway::Gateway(ActionSpmcReceiver &&actionReceiver) : actionReceiver(std::move(actionReceiver))
{
}

grpc::Status Gateway::ListSecurities(grpc::ServerContext *ctx, const google::protobuf::Empty *request,
                                     proto::ListSecuritiesResponse *response)
{
    for (const auto &security : common::Universe::getSecurities())
    {
        auto *securityPtr = response->add_securities();
        *securityPtr = security;
    }
    return grpc::Status::OK;
}

grpc::Status Gateway::GetSecurityById(grpc::ServerContext *context, const proto::GetSecurityByIdRequest *request,
                                      proto::Security *response)
{
    auto allSecurities = common::Universe::getSecurities();
    // NOLINTNEXTLINE(readability-qualified-auto) - std::array::iterator is not a pointer on all platforms
    const auto it = std::ranges::find_if(
        allSecurities, [&request](const auto &security) { return security.security_id() == request->security_id(); });
    if (it == allSecurities.end())
    {
        return {grpc::StatusCode::NOT_FOUND, "No security exists with the provided id."};
    }
    *response = *it;
    return grpc::Status::OK;
}

grpc::Status Gateway::GetSecurityBySymbol(grpc::ServerContext *context,
                                          const proto::GetSecurityBySymbolRequest *request, proto::Security *response)
{
    auto allSecurities = common::Universe::getSecurities();
    // NOLINTNEXTLINE(readability-qualified-auto) - std::array::iterator is not a pointer on all platforms
    const auto it = std::ranges::find_if(
        allSecurities, [&request](const auto &security) { return security.symbol() == request->symbol(); });
    if (it == allSecurities.end())
    {
        return {grpc::StatusCode::NOT_FOUND, "No security exists with the provided symbol."};
    }
    *response = *it;
    return grpc::Status::OK;
}

} // namespace exchange::server
