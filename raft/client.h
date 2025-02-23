//
// Created by pknadimp on 1/23/25.
//

#ifndef CLIENT_H
#define CLIENT_H

#include "error_codes.h"
#include <functional>
#include <string>

namespace raft {
    struct RequestVoteRPC {
        unsigned int term;
        unsigned int candidate_id;
        unsigned int last_log_index;
        unsigned int last_log_term;
    };

    struct RequestVoteResponse {
        unsigned int term;
        bool vote_granted;
        bool error;
    };

    struct AppendEntriesRPC {
        unsigned int term;
        unsigned int leader_id;
        unsigned int prev_log_index;
        unsigned int prev_log_term;
        std::string entries;
        unsigned int commit_index;
    };

    struct AppendEntriesResponse {
        unsigned int term;
        bool success;
        bool error;
    };

    struct ClientRequestRPC {
        std::string command;
    };

    struct ClientRequestResponse {
        bool success;
        bool redirect;
        unsigned int leader_id;
        std::string data;
    };

    // Client Interface
    class Client {
    public:
        virtual ~Client() = 0;

        // This method is responsible for requesting a vote from peer
        // and processing callback once complete
        virtual void request_vote(std::string address, const RequestVoteRPC &request_vote_rpc,
                                  std::function<void(RequestVoteResponse)> callback) = 0;

        // this method is responsible for sending a append entries
        // request and processing call back once request is complete
        virtual void append_entries(std::string address, const AppendEntriesRPC &append_entries,
                            std::function<void(AppendEntriesResponse)> callback) = 0;

    };
}


#endif //CLIENT_H
