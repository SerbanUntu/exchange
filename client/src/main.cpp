#include <client/main.hpp>

#include "client/cli_parsing.hpp"
#include "common/proto/proto_declarations.hpp"
#include "common/util/logging.hpp"

#include <grpcpp/grpcpp.h>

#include <chrono>
#include <thread>

namespace exchange::client
{

int main(int argc, char **argv) noexcept
{
    std::string serverUrl = "localhost:8989";
    uint32_t securityId;
    proto::Side side;
    uint64_t quantity;
    uint64_t priceCents;
    proto::TimeInForce tif;
    std::string orderId;
    uint32_t resolutionSeconds;
    std::string startTime;
    std::string endTime;
    uint32_t numberOfLevels;
    std::string symbol;

    parseArgs(argc, argv, serverUrl, securityId, side, quantity, priceCents, tif, orderId, resolutionSeconds, startTime,
              endTime, numberOfLevels, symbol);

    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        constexpr int NUMBER_OF_REQUESTS = 1000;
        LOG = common::LogService::getLogger(common::LogProducer::CLIENT);
        LOG->info("Starting the exchange client.");

        const auto channel = grpc::CreateChannel(serverUrl, grpc::InsecureChannelCredentials());
        const auto stub = proto::Gateway::NewStub(channel);

        for (auto i = 0; i < NUMBER_OF_REQUESTS; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            google::protobuf::Empty emptyRequest;

            proto::ListSecuritiesResponse response;
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
        common::reportException(LOG, err);
        return 1;
    }
    catch (...)
    {
        common::reportUnknownException(LOG);
        return 1;
    }
}
} // namespace exchange::client
