#pragma once
#include "common/proto/proto_declarations.hpp"

#include <cstdint>
#include <string>

namespace exchange::client
{

int parseArgs(int argc, char **argv, std::string &serverUrl, uint32_t &securityId, proto::Side &side,
              uint64_t &quantity, uint64_t &priceCents, proto::TimeInForce &tif, std::string &orderId,
              uint32_t &resolutionSeconds, std::string &startTime, std::string &endTime, uint32_t &numberOfLevels,
              std::string &symbol);

} // namespace exchange::client