#pragma once
#include "common/proto/proto_declarations.hpp"

#include <cstdint>
#include <string>
#include <optional>
#include <variant>

namespace exchange::client
{

struct OrderLimitResult
{
    uint32_t securityId;
    uint64_t priceCents;
    uint64_t quantity;
    proto::Side side;
    proto::TimeInForce tif;
};

struct OrderMarketResult
{
    uint32_t securityId;
    proto::Side side;
    uint64_t quantity;
};

struct OrderCancelResult
{
    std::string orderId;
};

struct OrderAmendResult
{
    std::string orderId;
    std::optional<uint64_t> priceCents;
    std::optional<uint64_t> quantity;
};

struct DataCandlesResult
{
    uint32_t securityId;
    uint32_t resolutionSeconds;
    std::optional<std::string> startTime;
    std::optional<std::string> endTime;
};

struct DataBookL1Result
{
    uint32_t securityId;
};

struct DataBookL2Result
{
    uint32_t securityId;
    std::optional<uint32_t> numberOfLevels;
};

struct DataBookL3Result
{
    uint32_t securityId;
};

struct DataOrderResult
{
    std::string orderId;
};

struct DataExecResult
{
    std::optional<uint32_t> securityId;
    std::optional<std::string> orderId;
    std::optional<std::string> startTime;
    std::optional<std::string> endTime;
};

struct SecurityListResult
{
};

struct SecurityViewResult
{
    using SecurityId = uint32_t;
    using Symbol = std::string;
    std::variant<SecurityId, Symbol> key;
};

struct ParseCommandResult
{
    std::variant<std::monostate, OrderLimitResult, OrderMarketResult, OrderCancelResult, OrderAmendResult,
                 DataCandlesResult, DataBookL1Result, DataBookL2Result, DataBookL3Result, DataOrderResult,
                 DataExecResult, SecurityListResult, SecurityViewResult>
        command;
    int exitCode{};
};

/**
 * Parse the raw command line arguments into the parameters needed to build a specific gRPC request for the exchange
 * server.
 *
 * @param argc The number of command line arguments.
 * @param argv The values of the command line arguments.
 * @param serverUrl A mutable reference to the URL of the server to contact (if not specified, it defaults to
 * localhost:8989).
 * @return Details about the command that was parsed from the command line (whether there was an error, and if there was
 * no error, which command was chosen).
 */
ParseCommandResult parseCommand(int argc, char **argv, std::string &serverUrl);

} // namespace exchange::client