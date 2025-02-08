//
// Created by pknadimp on 2/6/25.
//

#ifndef GRPC_SERVER_CPP_H
#define GRPC_SERVER_CPP_H

#include <proto/raft.grpc.pb.h>
#include <grpcpp/grpcpp.h>
#include "event_queue.h"
#include "events.h"


namespace raft {
    class RaftSeverImpl final : public raft_gRPC::RaftService::Service {
    public:
       explicit RaftSeverImpl(EventQueue<Event> &event_queue)
            : m_event_queue(event_queue) {
            std::cout << "RaftServerImpl constructed" << std::endl;
        };

        ~RaftSeverImpl() {
            std::cout << "RaftServerImpl destroyed" << std::endl;
        }

        grpc::Status HandleVoteRequest(grpc::ServerContext *context, const raft_gRPC::RequestVote *request_vote,
                                       raft_gRPC::RequestVoteResponse *response) override;

    private:
        EventQueue<Event> &m_event_queue;

    };
}

#endif //GRPC_SERVER_CPP_H
