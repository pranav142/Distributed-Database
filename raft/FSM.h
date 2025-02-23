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

        // Applies a command to the FSM
        // that can modify the state
        virtual FSMResponse apply_command(const std::string &serialized_command) = 0;

        // Applies a command to the FSM
        // that doesn't modify the state
        virtual FSMResponse query_state(const std::string &serialized_command) = 0;

        // indicates if a command modifies the state of the
        // finite state machine so raft can process accordingly
        virtual bool is_modifying_command(const std::string &serialized_command) = 0;
    };
}


#endif //FSM_H
