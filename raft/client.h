//
// Created by pknadimp on 1/23/25.
//

#ifndef CLIENT_H
#define CLIENT_H

#include "error_codes.h"
#include <functional>
#include <string>

namespace raft {
    // Virtual Class that enables raft to use a variety
    // of networking protocols, and enables the tester to effectively
    // Mock responses
    struct RequestVoteRPC {
        unsigned int term;
        unsigned int candidate_id;
        unsigned int last_log_index;
        unsigned int last_log_term;
    };

    struct RequestVoteResponse {
        int term; // term of the voter
        bool vote_granted;
    };

    class Client {
    public:
        virtual ~Client() = 0;

        // This method is responsible for requesting a vote from peer
        // Once the request has been processed the callback will then
        // be invoked; this allows for simple asynchronous programming
        virtual void request_vote(std::string address, const RequestVoteRPC& request_vote_rpc,
                                  std::function<void(RequestVoteResponse)> callback) = 0;
    };
}


#endif //CLIENT_H
