//
// Created by pknadimp on 2/6/25.
//

#include "gRPC_server.h"

#include <absl/log/check.h>

void raft::gRPCServer::RequestVoteCallData::proceed() {
    if (m_status == CREATE) {
        m_status = PROCESS;

        m_service->RequestHandleVoteRequest(&m_ctx, &m_request, &m_responder, m_cq, m_cq, this);
    } else if (m_status == PROCESS) {
        // create a new request vote call to indicate
        // we can handle another one
        new RequestVoteCallData(m_service, m_cq, m_event_queue);

        RequestVoteEvent request_vote_event{};
        request_vote_event.candidate_id = m_request.candidate_id();
        request_vote_event.last_log_index = m_request.last_log_index();
        request_vote_event.last_log_term = m_request.last_log_term();
        request_vote_event.term = m_request.term();
        request_vote_event.callback = [this](const RequestVoteResponse &_response) {
            m_status = FINISH;
            m_reply.set_term(_response.term);
            m_reply.set_vote_granted(_response.vote_granted);
            m_responder.Finish(m_reply, grpc::Status::OK, this);
        };
        m_event_queue.push(request_vote_event);
    } else {
        CHECK_EQ(m_status, FINISH);
        delete this;
    }
}

void raft::gRPCServer::AppendEntryCallData::proceed() {
    if (m_status == CREATE) {
        m_status = PROCESS;

        m_service->RequestHandleAppendEntries(&m_ctx, &m_request, &m_responder, m_cq, m_cq, this);
    } else if (m_status == PROCESS) {
        // create new append entry call
        // to indicate we can handle another
        new AppendEntryCallData(m_service, m_cq, m_event_queue);

        AppendEntriesEvent append_entries_event{};
        append_entries_event.commit_index = m_request.commit_index();
        append_entries_event.entries = m_request.entries();
        append_entries_event.term = m_request.term();
        append_entries_event.prev_log_index = m_request.prev_log_index();
        append_entries_event.prev_log_term = m_request.prev_log_term();
        append_entries_event.leader_id = m_request.leader_id();
        append_entries_event.callback = [this](const AppendEntriesResponse &_response) {
            m_status = FINISH;
            m_reply.set_term(_response.term);
            m_reply.set_success(_response.success);
            m_responder.Finish(m_reply, grpc::Status::OK, this);
        };

        m_event_queue.push(append_entries_event);
    } else {
        CHECK_EQ(m_status, FINISH);
        delete this;
    }
}

void raft::gRPCServer::handle_rpcs() {
    new AppendEntryCallData(&m_service, m_cq.get(), m_event_queue);
    new RequestVoteCallData(&m_service, m_cq.get(), m_event_queue);

    void* tag;
    bool ok;
    while (m_cq->Next(&tag, &ok) && ok) {
      static_cast<CallDataBase*>(tag)->proceed();
    }
}