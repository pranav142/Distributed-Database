//
// Created by pknadimp on 1/20/25.
//

#ifndef EVENTS_H
#define EVENTS_H

#include <variant>

struct ElectionTimeout {
};

typedef std::variant<ElectionTimeout>
Event;

#endif //EVENTS_H
