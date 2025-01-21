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

typedef std::variant<ElectionTimeout, QuitEvent> Event;

#endif //EVENTS_H
