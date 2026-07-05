#include <server/main.hpp>

#include "common/util/logging.hpp"

#include <exchange.pb.h>
#include <exchange.grpc.pb.h>

#include <grpcpp/grpcpp.h>

namespace exchange::server
{
class OrderServiceImpl final : public OrderService::Service
{
  public:
    grpc::Status SubmitOrder(grpc::ServerContext *ctx, const OrderRequest *request, OrderResponse *response) override
    {
        const auto LOG = common::util::LogService::getLogger(common::util::LogProducer::ORDER_MANAGER);
        LOG->info("Received order for {} of {}.", request->quantity(), request->symbol());
        response->set_success(true);
        LOG->info("Order accepted.");
        return grpc::Status::OK;
    }
};

int main() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::util::LogService::getLogger(common::util::LogProducer::ORDER_MANAGER);
        LOG->info("Starting the exchange server.");

        const std::string address = "0.0.0.0:8989";
        OrderServiceImpl service;

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
        common::util::reportException(LOG, err);
        return 1;
    }
    catch (...)
    {
        common::util::reportUnknownException(LOG);
        return 1;
    }
}
} // namespace exchange::server

int main()
{
    return exchange::server::main();
}
