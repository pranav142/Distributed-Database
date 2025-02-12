//
// Created by pknadimp on 1/27/25.
//

#include "gRPC_client.h"
#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

void raft::gRPCClient::request_vote(std::string address, const RequestVoteRPC &request_vote_rpc,
                                    std::function<void(RequestVoteResponse)> callback) {
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    auto call = new AsyncCall<RequestVoteResponse, raft_gRPC::RequestVoteResponse>;
    call->callback = callback;
    call->stub = raft_gRPC::RaftService::NewStub(channel);
    call->type = RPCType::REQUEST_VOTE;

    raft_gRPC::RequestVote request;
    request.set_term(request_vote_rpc.term);
    request.set_candidate_id(request_vote_rpc.candidate_id);
    request.set_last_log_index(request_vote_rpc.last_log_index);
    request.set_last_log_term(request_vote_rpc.last_log_term);

    call->response_reader = call->stub->AsyncHandleVoteRequest(&call->context, request, m_cq.get());
    call->response_reader->Finish(&call->reply, &call->status, static_cast<void *>(call));
}

void raft::gRPCClient::handle_request_vote_rpc(
    const AsyncCall<RequestVoteResponse, raft_gRPC::RequestVoteResponse> *call, bool ok) const {
    RequestVoteResponse response{};
    if (ok && call->status.ok()) {
        response.term = static_cast<int>(call->reply.term());
        response.vote_granted = call->reply.vote_granted();
        response.error = false;
    } else {
        response.term = 0;
        response.vote_granted = false;
        response.error = true;
    }
    call->callback(response);
}

void raft::gRPCClient::append_entries(std::string address, const AppendEntriesRPC &append_entries_rpc,
                                      std::function<void(AppendEntriesResponse)> callback) {
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    auto call = new AsyncCall<AppendEntriesResponse, raft_gRPC::AppendEntriesResponse>;
    call->callback = callback;
    call->stub = raft_gRPC::RaftService::NewStub(channel);
    call->type = RPCType::APPEND_ENTRIES;

    raft_gRPC::AppendEntries request;
    request.set_term(append_entries_rpc.term);
    request.set_entries(append_entries_rpc.entries);
    request.set_leader_id(append_entries_rpc.leader_id);
    request.set_commit_index(append_entries_rpc.commit_index);
    request.set_prev_log_index(append_entries_rpc.prev_log_index);
    request.set_prev_log_term(append_entries_rpc.prev_log_term);

    call->response_reader = call->stub->AsyncHandleAppendEntries(&call->context, request, m_cq.get());
    call->response_reader->Finish(&call->reply, &call->status, static_cast<void *>(call));
}

void raft::gRPCClient::handle_append_entries_rpc(
    const AsyncCall<AppendEntriesResponse, raft_gRPC::AppendEntriesResponse> *call, bool ok) const {
    AppendEntriesResponse response{};

    if (ok && call->status.ok()) {
        response.term = call->reply.term();
        response.success = call->reply.success();
    } else {
        response.term = 0;
        response.success = false;
    }

    call->callback(response);
}

void raft::gRPCClient::async_complete_rpc() const {
    void *tag;
    bool ok;
    while (m_cq->Next(&tag, &ok)) {
        auto call_base = static_cast<AsyncCallBase *>(tag);
        if (call_base->type == RPCType::REQUEST_VOTE) {
            auto call = static_cast<AsyncCall<RequestVoteResponse, raft_gRPC::RequestVoteResponse> *>(call_base);
            handle_request_vote_rpc(call, ok);
        } else if (call_base->type == RPCType::APPEND_ENTRIES) {
            auto call = static_cast<AsyncCall<AppendEntriesResponse, raft_gRPC::AppendEntriesResponse> *>(call_base);
            handle_append_entries_rpc(call, ok);
        }
        delete call_base;
    }
}
