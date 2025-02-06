//
// Created by pknadimp on 1/27/25.
//

#ifndef GRPC_CLIENT_H
#define GRPC_CLIENT_H

#include "proto/raft.grpc.pb.h"
#include "client.h"

namespace raft {

    class gRPCClient : public Client {
    public:
        gRPCClient() = default;
        ~gRPCClient() override = default;

        void request_vote(std::string address, const RequestVoteRPC& request_vote_rpc, std::function<void(RequestVoteResponse)> callback) override;
    private:
         static raft_gRPC::RequestVote create_request_vote(unsigned int term, unsigned int id, unsigned int last_log_index,
                                    unsigned int last_log_term);
    };
}


#endif //GRPC_CLIENT_H
