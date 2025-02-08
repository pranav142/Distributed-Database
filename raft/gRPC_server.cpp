//
// Created by pknadimp on 2/6/25.
//

#include "gRPC_server.h"

grpc::Status raft::RaftSeverImpl::HandleVoteRequest(grpc::ServerContext *context,
                                                    const raft_gRPC::RequestVote *request_vote,
                                                    raft_gRPC::RequestVoteResponse *response) {
    std::mutex mtx;
    std::condition_variable cv;
    bool is_done = false;

    RequestVoteEvent request_vote_event{};
    request_vote_event.candidate_id = request_vote->candidate_id();
    request_vote_event.last_log_index = request_vote->last_log_index();
    request_vote_event.last_log_term = request_vote->last_log_term();
    request_vote_event.term = request_vote->term();

    request_vote_event.callback = [&response, &mtx, &cv, &is_done](const RequestVoteResponse &_response) {
        response->set_term(_response.term);
        response->set_vote_granted(_response.vote_granted); {
            std::lock_guard lock(mtx);
            is_done = true;
        }
        cv.notify_one();
        std::cout << "callback" << std::endl;
    };

    m_event_queue.push(request_vote_event);

    // waits until the response has been populated
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return is_done; });
    }

    std::cout << "finished" << std::endl;
    return grpc::Status::OK;
}
