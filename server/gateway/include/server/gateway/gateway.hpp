#pragma once
#include "common/proto/proto_declarations.hpp"
#include "server/common/collections/ipc/action_spmc_receiver.hpp"

namespace exchange::server
{
class Gateway final : public proto::Gateway::Service
{
    ActionSpmcReceiver actionReceiver;

  public:
    explicit Gateway(ActionSpmcReceiver &&actionReceiver);

    grpc::Status ListSecurities(grpc::ServerContext *ctx, const google::protobuf::Empty *request,
                                proto::ListSecuritiesResponse *response) override;

    grpc::Status GetSecurityById(grpc::ServerContext *context, const proto::GetSecurityByIdRequest *request,
                                 proto::Security *response) override;

    grpc::Status GetSecurityBySymbol(grpc::ServerContext *context, const proto::GetSecurityBySymbolRequest *request,
                                     proto::Security *response) override;
};

} // namespace exchange::server
