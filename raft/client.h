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
    class Client {
    public:
        virtual ~Client() = default;

        // This method is responsible for requesting a vote from peer
        // Once the request has been processed the callback will then
        // be invoked; this allows for simple asynchronous programming
        virtual void request_vote(std::string address, int term, int candidate_id, int last_log_index, int last_log_term,
                                       std::function<void(int term, bool vote_granted)> callback) = 0;

        // Create a method to cancel any outgoing requests
        // virtual void cancel();
    };
}


#endif //CLIENT_H
