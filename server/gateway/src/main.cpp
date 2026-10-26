#include <server/gateway/main.hpp>

#include "common/util/logging.hpp"
#include "server/common/collections/ipc/action_spmc_receiver.hpp"
#include "server/common/collections/ipc/shared_memory.hpp"
#include "server/gateway/gateway.hpp"

#include <grpcpp/grpcpp.h>

namespace exchange::server
{
int gatewayMain() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::LogService::getLogger(common::LogProducer::GATEWAY);
        LOG->info("Starting the exchange server gateway.");

        const std::string address = "0.0.0.0:8989";
        SharedMemory sharedMemory{false};
        ActionSpmcReceiver spmcReceiver{sharedMemory};
        Gateway service{std::move(spmcReceiver)};

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
