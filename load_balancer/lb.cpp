//
// Created by pknadimp on 2/14/25.
//

#include "lb.h"
#include <grpcpp/grpcpp.h>

bool command_request(const std::string &address, const raft_gRPC::ClientRequest &request,
                     raft_gRPC::ClientResponse *response) {
    grpc::ClientContext context;
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    auto stub = raft_gRPC::RaftService::NewStub(channel);
    grpc::Status status = stub->HandleClientRequest(&context, request, response);
    return status.ok();
}
