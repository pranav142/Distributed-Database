//
// Created by pknadimp on 1/27/25.
//

#include "gRPC_client.h"
#include <grpcpp/grpcpp.h>

void raft::gRPCClient::request_vote(std::string address, const RequestVoteRPC &request_vote_rpc,
                                    std::function<void(RequestVoteResponse)> callback) {
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());

    auto call = new AsyncCall;
    call->callback = callback;
    call->address = address;
    call->stub = raft_gRPC::RaftService::NewStub(channel);

    raft_gRPC::RequestVote request;
    request.set_term(request_vote_rpc.term);
    request.set_candidate_id(request_vote_rpc.candidate_id);
    request.set_last_log_index(request_vote_rpc.last_log_index);
    request.set_last_log_term(request_vote_rpc.last_log_term);

    call->response_reader = call->stub->AsyncHandleVoteRequest(&call->context, request, m_cq.get());
    call->response_reader->Finish(&call->reply, &call->status, static_cast<void *>(call));
}

void raft::gRPCClient::async_complete_rpc() const {
    void *tag;
    bool ok;
    while (m_cq->Next(&tag, &ok)) {
        auto *call = static_cast<AsyncCall *>(tag);

        RequestVoteResponse response;
        if (ok && call->status.ok()) {
            response.term = static_cast<int>(call->reply.term());
            response.vote_granted = call->reply.vote_granted();
        } else {
            response.term = 0;
            response.vote_granted = false;
        }
        call->callback(response);

        delete call;
    }
}