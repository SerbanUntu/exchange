#include <client/main.hpp>

#include "client/cli_parsing.hpp"
#include "client/response_displayer.hpp"
#include "common/proto/proto_declarations.hpp"
#include "common/util/logging.hpp"
#include "common/util/overloads.hpp"

#include <google/protobuf/util/time_util.h>
#include <grpcpp/grpcpp.h>

namespace exchange::client
{

int main(const int argc, char **argv) noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::LogService::getLogger(common::LogProducer::CLIENT);
        std::string serverUrl = "localhost:8989";

        const auto parseCommandResult = parseCommand(argc, argv, serverUrl);

        if (std::holds_alternative<std::monostate>(parseCommandResult.command))
        {
            return parseCommandResult.exitCode;
        }

        LOG->info("Creating the gRPC channel...");
        const auto channel = grpc::CreateChannel(serverUrl, grpc::InsecureChannelCredentials());
        const auto stub = proto::Gateway::NewStub(channel);
        LOG->info("gRPC channel created.");

        proto::ListSecuritiesResponse response;
        grpc::ClientContext context;
        ResponseDisplayer responseDisplayer;

        using google::protobuf::util::TimeUtil;
        constexpr uint32_t MAX_L2_LEVELS = 10;

        const auto printIfOk = [&responseDisplayer](const grpc::Status &rpcStatus, const auto &rpcResponse) {
            if (rpcStatus.ok())
            {
                responseDisplayer.printResponse(rpcResponse);
            }
            return rpcStatus;
        };

        const grpc::Status status = std::visit(
            overloads{
                [](std::monostate) {
                    return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT, "no command was parsed"};
                },
                [&](const OrderLimitResult &command) {
                    proto::SubmitOrderRequest request;
                    request.set_security_id(command.securityId);
                    request.set_type(proto::OrderType::ORDER_TYPE_LIMIT);
                    request.set_side(command.side);
                    request.set_quantity(command.quantity);
                    request.set_price(command.priceCents);
                    request.set_tif(command.tif);
                    proto::OrderAck ack;
                    return printIfOk(stub->SubmitOrder(&context, request, &ack), ack);
                },
                [&](const OrderMarketResult &command) {
                    proto::SubmitOrderRequest request;
                    request.set_security_id(command.securityId);
                    request.set_type(proto::OrderType::ORDER_TYPE_MARKET);
                    request.set_side(command.side);
                    request.set_quantity(command.quantity);
                    proto::OrderAck ack;
                    return printIfOk(stub->SubmitOrder(&context, request, &ack), ack);
                },
                [&](const OrderCancelResult &command) {
                    proto::SingleOrderRequest request;
                    request.set_order_id(command.orderId);
                    proto::OrderAck ack;
                    return printIfOk(stub->CancelOrder(&context, request, &ack), ack);
                },
                [&](const OrderAmendResult &command) {
                    proto::AmendOrderRequest request;
                    request.set_order_id(command.orderId);
                    if (command.priceCents)
                    {
                        request.set_new_price(*command.priceCents);
                    }
                    if (command.quantity)
                    {
                        request.set_new_quantity(*command.quantity);
                    }
                    proto::OrderAck ack;
                    return printIfOk(stub->AmendOrder(&context, request, &ack), ack);
                },
                [&](const DataCandlesResult &command) {
                    proto::CandleDataRequest request;
                    request.set_security_id(command.securityId);
                    request.set_resolution_seconds(command.resolutionSeconds);
                    if (command.startTime)
                    {
                        TimeUtil::FromString(*command.startTime, request.mutable_start_time());
                    }
                    if (command.endTime)
                    {
                        TimeUtil::FromString(*command.endTime, request.mutable_end_time());
                    }
                    proto::CandleDataResponse candles;
                    return printIfOk(stub->GetCandleData(&context, request, &candles), candles);
                },
                [&](const DataBookL1Result &command) {
                    proto::OrderBookDataRequest request;
                    request.set_security_id(command.securityId);
                    proto::L1DataResponse l1Data;
                    return printIfOk(stub->GetL1Data(&context, request, &l1Data), l1Data);
                },
                [&](const DataBookL2Result &command) {
                    proto::L2DataRequest request;
                    request.set_security_id(command.securityId);
                    request.set_number_of_levels(command.numberOfLevels.value_or(MAX_L2_LEVELS));
                    proto::L2DataResponse l2Data;
                    return printIfOk(stub->GetL2Data(&context, request, &l2Data), l2Data);
                },
                [&](const DataBookL3Result &command) {
                    proto::OrderBookDataRequest request;
                    request.set_security_id(command.securityId);
                    const auto reader = stub->GetL3Data(&context, request);
                    proto::OrderBookUpdate update;
                    while (reader->Read(&update))
                    {
                        responseDisplayer.printResponse(update);
                    }
                    return reader->Finish();
                },
                [&](const DataOrderResult &command) {
                    proto::SingleOrderRequest request;
                    request.set_order_id(command.orderId);
                    proto::OrderDataResponse order;
                    return printIfOk(stub->GetOrderData(&context, request, &order), order);
                },
                [&](const DataExecResult &command) {
                    proto::ExecutionDataRequest request;
                    if (command.securityId)
                    {
                        request.set_security_id(*command.securityId);
                    }
                    if (command.orderId)
                    {
                        request.set_order_id(*command.orderId);
                    }
                    if (command.startTime)
                    {
                        TimeUtil::FromString(*command.startTime, request.mutable_start_time());
                    }
                    if (command.endTime)
                    {
                        TimeUtil::FromString(*command.endTime, request.mutable_end_time());
                    }
                    proto::ExecutionDataResponse executionData;
                    return printIfOk(stub->GetExecutionData(&context, request, &executionData), executionData);
                },
                [&](const SecurityListResult &) {
                    return printIfOk(stub->ListSecurities(&context, google::protobuf::Empty{}, &response), response);
                },
                [&](const SecurityViewResult &command) {
                    proto::Security security;
                    return std::visit(
                        overloads{
                            [&](const SecurityViewResult::SecurityId securityId) {
                                proto::GetSecurityByIdRequest request;
                                request.set_security_id(securityId);
                                return printIfOk(stub->GetSecurityById(&context, request, &security), security);
                            },
                            [&](const SecurityViewResult::Symbol &symbol) {
                                proto::GetSecurityBySymbolRequest request;
                                request.set_symbol(symbol);
                                return printIfOk(stub->GetSecurityBySymbol(&context, request, &security), security);
                            },
                        },
                        command.key);
                },
            },
            parseCommandResult.command);

        if (!status.ok())
        {
            LOG->error("RPC failed: {}.", status.error_message());
            return 1;
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
