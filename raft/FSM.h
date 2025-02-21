//
// Created by pknadimp on 2/15/25.
//

#ifndef FSM_H
#define FSM_H

#include <string>

namespace raft {
    struct FSMResponse {
        bool success;
        std::string data;
    };

    class FSM {
    public:
        virtual ~FSM() = default;

        virtual FSMResponse apply_command(const std::string &serialized_command) = 0;

        virtual FSMResponse query_state(const std::string &serialized_command) = 0;
    };
}


#endif //FSM_H
