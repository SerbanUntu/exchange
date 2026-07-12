#include <client/main.hpp>

#include "common/util/logging.hpp"

#include <gateway.pb.h>
#include <gateway.grpc.pb.h>

#include <grpcpp/grpcpp.h>

#include <chrono>
#include <thread>

namespace exchange::client
{
using namespace v1;

int main() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        constexpr int NUMBER_OF_REQUESTS = 1000;
        LOG = common::util::LogService::getLogger(common::util::LogProducer::CLIENT);
        LOG->info("Starting the exchange client.");

        const auto channel = grpc::CreateChannel("localhost:8989", grpc::InsecureChannelCredentials());
        const auto stub = Gateway::NewStub(channel);

        for (auto i = 0; i < NUMBER_OF_REQUESTS; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            google::protobuf::Empty emptyRequest;

            ListSecuritiesResponse response;
            grpc::ClientContext context;

            if (const grpc::Status status = stub->ListSecurities(&context, emptyRequest, &response); status.ok())
            {
                LOG->info("Response received: {} securities.", response.securities_size());
            }
            else
            {
                LOG->error("RPC failed: {}.", status.error_message());
            }
        }
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
} // namespace exchange::client

int main()
{
    return exchange::client::main();
}
