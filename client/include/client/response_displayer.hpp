#pragma once
#include "common/proto/proto_declarations.hpp"

#include <functional>
#include <iostream>

namespace exchange::client
{

class ResponseDisplayer
{
    const std::reference_wrapper<std::ostream> out;

    static std::string toString(const google::protobuf::Timestamp &timestamp);
    static std::string toString(proto::Side side);
    static std::string toString(proto::OrderStatus status);
    static std::string priceToString(uint64_t price);

  public:
    ResponseDisplayer();
    explicit ResponseDisplayer(std::ostream &outputStream);
    [[nodiscard]] std::ostream &getOutputStream() const;

    void printResponse(const proto::OrderAck &ack) const;
    void printResponse(const proto::CandleDataResponse &candles) const;
    void printResponse(const proto::L1DataResponse &l1Data) const;
    void printResponse(const proto::L2DataResponse &l2Data) const;
    void printResponse(const proto::OrderBookUpdate &update) const;
    void printResponse(const proto::OrderDataResponse &order) const;
    void printResponse(const proto::ExecutionDataResponse &executionData) const;
    void printResponse(const proto::ListSecuritiesResponse &securities) const;
    void printResponse(const proto::Security &security) const;
};

} // namespace exchange::client