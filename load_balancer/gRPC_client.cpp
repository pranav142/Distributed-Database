//
// Created by pknadimp on 3/7/25.
//

#include "gRPC_client.h"

#include <grpcpp/grpcpp.h>

bool loadbalancer::send_request_grpc(const std::string &address, const raft_gRPC::ClientRequest &request,
                                     raft_gRPC::ClientResponse *response) {
    grpc::ClientContext context;
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    auto stub = raft_gRPC::RaftService::NewStub(channel);
    grpc::Status status = stub->HandleClientRequest(&context, request, response);
    return status.ok();
}
