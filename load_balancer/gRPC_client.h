//
// Created by pknadimp on 3/7/25.
//

#ifndef LB_GRPC_CLIENT_H
#define LB_GRPC_CLIENT_H

#include "proto/raft.grpc.pb.h"

namespace loadbalancer {
    // TODO: Introduce a Asynchronous method

    bool send_request_grpc(const std::string &address, const raft_gRPC::ClientRequest &request,
                           raft_gRPC::ClientResponse *response);
} // loadbalancer

#endif //LB_GRPC_CLIENT_H
