//
// Created by pknadimp on 2/14/25.
//

#ifndef LB_H
#define LB_H

#include <proto/raft.grpc.pb.h>
#include <string>

bool command_request(const std::string &address,
                     const raft_gRPC::ClientRequest &request,
                     raft_gRPC::ClientResponse *response);

#endif //LB_H
