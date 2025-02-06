//
// Created by pknadimp on 1/20/25.
//

#ifndef EVENTS_H
#define EVENTS_H

#include <variant>

struct QuitEvent {
};

struct ElectionTimeout {
};

struct RequestVoteResponse {
    int term;
    bool vote_granted;
};

typedef std::variant<ElectionTimeout, QuitEvent, RequestVoteResponse> Event;

#endif //EVENTS_H
