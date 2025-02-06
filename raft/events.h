//
// Created by pknadimp on 1/20/25.
//

#ifndef EVENTS_H
#define EVENTS_H

#include <variant>

namespace raft {
    struct QuitEvent {
    };

    struct ElectionTimeoutEvent {
    };

    struct RequestVoteResponseEvent {
        int term;
        bool vote_granted;
        bool success;
        std::string address;
    };

    typedef std::variant<ElectionTimeoutEvent, QuitEvent, RequestVoteResponseEvent> Event;
}

#endif //EVENTS_H
