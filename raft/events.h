//
// Created by pknadimp on 1/20/25.
//

#ifndef EVENTS_H
#define EVENTS_H

#include <variant>

#include "client.h"

namespace raft {
    struct QuitEvent {
    };

    struct ElectionTimeoutEvent {
    };

    struct RequestVoteResponseEvent {
        int term;
        bool vote_granted;
    };

    struct RequestVoteEvent {
        unsigned int term;
        unsigned int candidate_id;
        unsigned int last_log_index;
        unsigned int last_log_term;
        std::function<void(RequestVoteResponse)> callback;
    };

    typedef std::variant<ElectionTimeoutEvent, QuitEvent, RequestVoteResponseEvent, RequestVoteEvent> Event;
}

#endif //EVENTS_H
