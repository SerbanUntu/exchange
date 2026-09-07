#include <server/gateway/main.hpp>

#include "common/model/universe.hpp"
#include "common/util/logging.hpp"

#include <gateway.pb.h>
#include <gateway.grpc.pb.h>

#include <grpcpp/grpcpp.h>

namespace exchange::server
{
using namespace v1;

class GatewayImpl final : public Gateway::Service
{
  public:
    grpc::Status ListSecurities(grpc::ServerContext *ctx, const google::protobuf::Empty *request,
                                ListSecuritiesResponse *response) override
    {
        for (const auto &security : common::Universe::getSecurities())
        {
            auto *securityPtr = response->add_securities();
            *securityPtr = security;
        }
        return grpc::Status::OK;
    }

    grpc::Status GetSecurityById(grpc::ServerContext *context, const GetSecurityByIdRequest *request,
                                 Security *response) override
    {
        auto allSecurities = common::Universe::getSecurities();
        // NOLINTNEXTLINE(readability-qualified-auto) - std::array::iterator is not a pointer on all platforms
        const auto it = std::ranges::find_if(allSecurities, [&request](const auto &security) {
            return security.security_id() == request->security_id();
        });
        if (it == allSecurities.end())
        {
            return {grpc::StatusCode::NOT_FOUND, "No security exists with the provided id."};
        }
        *response = *it;
        return grpc::Status::OK;
    }

    grpc::Status GetSecurityBySymbol(grpc::ServerContext *context, const GetSecurityBySymbolRequest *request,
                                     Security *response) override
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
};

int gatewayMain() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::LogService::getLogger(common::LogProducer::GATEWAY);
        LOG->info("Starting the exchange server gateway.");

        const std::string address = "0.0.0.0:8989";
        GatewayImpl service;

        grpc::ServerBuilder builder;
        builder.AddListeningPort(address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);

        const std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
        LOG->info("Listening on {}.", address);
        server->Wait();
        return 0;
    }
    catch (const std::exception &err)
    {
        common::reportException(LOG, err);
        return 1;
    }
    catch (...)
    {
        common::reportUnknownException(LOG);
        return 1;
    }
}
} // namespace exchange::server
