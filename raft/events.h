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

    struct HeartBeatEvent {
    };

    struct RequestVoteResponseEvent {
        unsigned int term;
        bool vote_granted;
    };

    struct RequestVoteEvent {
        unsigned int term;
        unsigned int candidate_id;
        unsigned int last_log_index;
        unsigned int last_log_term;
        std::function<void(RequestVoteResponse)> callback;
    };

    struct AppendEntriesEvent {
        unsigned int term;
        unsigned int leader_id;
        unsigned int prev_log_index;
        unsigned int prev_log_term;
        std::string entries;
        unsigned int commit_index;
        std::function<void(AppendEntriesResponse)> callback;
    };

    struct AppendEntriesResponseEvent {
        unsigned int term;
        bool success;
    };

    typedef std::variant<ElectionTimeoutEvent, QuitEvent, RequestVoteResponseEvent, RequestVoteEvent, AppendEntriesEvent
        , HeartBeatEvent, AppendEntriesResponseEvent> Event;
}

#endif //EVENTS_H
