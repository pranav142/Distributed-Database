//
// Created by pknadimp on 1/27/25.
//

#include "gRPC_client.h"
#include <grpcpp/grpcpp.h>

void raft::gRPCClient::request_vote(std::string address, const RequestVoteRPC &request_vote_rpc,
                                    std::function<void(RequestVoteResponse)> callback) {
    grpc::ClientContext context;
    raft_gRPC::RequestVoteResponse gRPC_response;
    raft_gRPC::RequestVote vote = create_request_vote(request_vote_rpc.term, request_vote_rpc.candidate_id,
                                                      request_vote_rpc.last_log_index, request_vote_rpc.last_log_term);
    auto channel = CreateChannel(address, grpc::InsecureChannelCredentials());
    auto stub = raft_gRPC::RaftService::NewStub(channel);

    stub->async()->HandleVoteRequest(&context, &vote, &gRPC_response,
                                     [callback, &gRPC_response, address](const grpc::Status &status) {
                                         RequestVoteResponse request_vote_response;
                                         request_vote_response.address = address;
                                         if (status.ok()) {
                                             request_vote_response.success = true;
                                             request_vote_response.term = static_cast<int>(gRPC_response.term());
                                             request_vote_response.vote_granted = gRPC_response.vote_granted();
                                         } else {
                                             // This specifies a failure has occurred
                                             request_vote_response.success = false;
                                             request_vote_response.term = -1;
                                             request_vote_response.vote_granted = false;
                                         }

                                         callback(request_vote_response);
                                     });
}

raft_gRPC::RequestVote raft::gRPCClient::create_request_vote(unsigned int term, unsigned int id,
                                                            unsigned int last_log_index,
                                                            unsigned int last_log_term) {
    raft_gRPC::RequestVote request_vote;
    request_vote.set_term(term);
    request_vote.set_candidate_id(id);
    request_vote.set_last_log_index(last_log_index);
    request_vote.set_last_log_term(last_log_term);
    return request_vote;
}
