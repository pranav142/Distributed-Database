//
// Created by pknadimp on 2/15/25.
//

#ifndef FSM_H
#define FSM_H

#include <string>

namespace raft {
    class FSM {
    public:
        virtual ~FSM() = default;

        virtual void apply_command(const std::string &serialized_command) = 0;
    };
}


#endif //FSM_H
