//
// Created by pknadimp on 1/27/25.
//

#include "gRPC_client.h"
#include <grpcpp/grpcpp.h>

void raft::gRPCClient::request_vote(std::string address, int term, int candidate_id, int last_log_index,
                                    int last_log_term, std::function<void(int term, bool vote_granted)> callback) {
    grpc::ClientContext context;
    RequestVoteResponse response;
    RequestVote vote = create_request_vote(term, candidate_id, last_log_index, last_log_term);
    auto channel = CreateChannel(address, grpc::InsecureChannelCredentials());
    auto stub = RaftService::NewStub(channel);

    stub->async()->HandleVoteRequest(&context, &vote, &response, [&callback, &response](const grpc::Status& status) {
        if (status.ok()) {
            callback(static_cast<int>(response.term()), response.vote_granted());
        }
        else {
            // This specifies a failure has occurred
            callback(-1, false);
        }
    });
}

raft::RequestVote raft::gRPCClient::create_request_vote(unsigned int term, unsigned int id, unsigned int last_log_index,
    unsigned int last_log_term) {
    RequestVote request_vote;
    request_vote.set_term(term);
    request_vote.set_candidate_id(id);
    request_vote.set_last_log_index(last_log_index);
    request_vote.set_last_log_term(last_log_term);
    return request_vote;
}
