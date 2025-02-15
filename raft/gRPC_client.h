//
// Created by pknadimp on 1/27/25.
//

#ifndef GRPC_CLIENT_H
#define GRPC_CLIENT_H

#include "proto/raft.grpc.pb.h"
#include "client.h"
#include <thread>

// TODO: Refactor to be cleaner
// TODO: introduce stub and channel caching to reduce overhead when communicating to nodes
namespace raft {
    enum class RPCType {
        REQUEST_VOTE,
        APPEND_ENTRIES,
    };

    struct AsyncCallBase {
        RPCType type;

        virtual ~AsyncCallBase() = default;
    };

    template<typename T, typename U>
    struct AsyncCall final : public AsyncCallBase {
        U reply;
        grpc::ClientContext context;
        grpc::Status status;
        std::unique_ptr<raft_gRPC::RaftService::Stub> stub;
        std::unique_ptr<grpc::ClientAsyncResponseReader<U> > response_reader;
        std::function<void(T)> callback;
    };

    class gRPCClient : public Client {
    public:
        gRPCClient() : m_cq(new grpc::CompletionQueue()) {
            m_worker_thread = std::thread(&gRPCClient::async_complete_rpc, this);
        }

        ~gRPCClient() override {
            m_cq->Shutdown();
            if (m_worker_thread.joinable()) {
                m_worker_thread.join();
            }
        }

        // Implements the request_vote method.
        // 'address' is the remote server (e.g., "localhost:50051"),
        // request_vote_rpc holds the vote request details,
        // and callback is the function to call once the RPC completes.
        void request_vote(std::string address, const RequestVoteRPC &request_vote_rpc,
                          std::function<void(RequestVoteResponse)> callback) override;

        void append_entries(std::string address, const AppendEntriesRPC &append_entries,
                            std::function<void(AppendEntriesResponse)> callback) override;

    private:
        void handle_request_vote_rpc(const AsyncCall<RequestVoteResponse, raft_gRPC::RequestVoteResponse> *call,
                                     bool ok) const;

        void handle_append_entries_rpc(const AsyncCall<AppendEntriesResponse, raft_gRPC::AppendEntriesResponse> *call,
                                       bool ok) const;

        void async_complete_rpc() const;

        std::unique_ptr<grpc::CompletionQueue> m_cq;
        std::thread m_worker_thread;
    };
}


#endif //GRPC_CLIENT_H
