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
    class gRPCServer {
    public:
        explicit gRPCServer(EventQueue<Event> &event_queue) : m_event_queue(event_queue) {
        }

        ~gRPCServer() {
            stop();
        }

        void stop() {
            if (m_server) {
                m_server->Shutdown();
            }

            if (m_cq) {
                m_cq->Shutdown();

                // drain the completion queue
                void *ignored_tag;
                bool ignored_ok;
                while (m_cq->Next(&ignored_tag, &ignored_ok)) {
                }
            }
        }

        void run(const std::string &server_address) {
            grpc::ServerBuilder builder;
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
            builder.RegisterService(&m_service);
            m_cq = builder.AddCompletionQueue();
            m_server = builder.BuildAndStart();
            std::cout << "Server listening on " << server_address << std::endl;
            handle_rpcs();
        }

    private:
        enum CallStatus { CREATE, PROCESS, FINISH };

        class CallDataBase {
        public:
            virtual ~CallDataBase() = default;

            virtual void proceed() = 0;
        };

        class RequestVoteCallData final : public CallDataBase {
        public:
            RequestVoteCallData(raft_gRPC::RaftService::AsyncService *service,
                                grpc::ServerCompletionQueue *cq, EventQueue<Event> &event_queue) : m_service(service),
                m_cq(cq), m_status(CREATE), m_responder(&m_ctx), m_event_queue(event_queue) {
                proceed();
            };

            void proceed() override;

        private:
            raft_gRPC::RaftService::AsyncService *m_service;
            grpc::ServerCompletionQueue *m_cq;
            grpc::ServerContext m_ctx;
            raft_gRPC::RequestVote m_request;
            raft_gRPC::RequestVoteResponse m_reply;

            grpc::ServerAsyncResponseWriter<raft_gRPC::RequestVoteResponse> m_responder;


            CallStatus m_status;
            EventQueue<Event> &m_event_queue;
        };

        class AppendEntryCallData final : public CallDataBase {
        public:
            AppendEntryCallData(raft_gRPC::RaftService::AsyncService *service,
                                grpc::ServerCompletionQueue *cq, EventQueue<Event> &event_queue) : m_service(service),
                m_cq(cq), m_status(CREATE), m_responder(&m_ctx), m_event_queue(event_queue) {
                proceed();
            }

            void proceed() override;;

        private:
            raft_gRPC::RaftService::AsyncService *m_service;
            grpc::ServerCompletionQueue *m_cq;
            grpc::ServerContext m_ctx;
            raft_gRPC::AppendEntries m_request;
            raft_gRPC::AppendEntriesResponse m_reply;

            grpc::ServerAsyncResponseWriter<raft_gRPC::AppendEntriesResponse> m_responder;

            CallStatus m_status;
            EventQueue<Event> &m_event_queue;
        };

        void handle_rpcs();

        std::unique_ptr<grpc::ServerCompletionQueue> m_cq;
        raft_gRPC::RaftService::AsyncService m_service;
        std::unique_ptr<grpc::Server> m_server;
        EventQueue<Event> &m_event_queue;
    };
}

#endif //GRPC_SERVER_CPP_H
