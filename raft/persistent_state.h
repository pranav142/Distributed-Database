//
// Created by pknadimp on 1/16/25.
//

#ifndef PERSISTENT_STATE_H
#define PERSISTENT_STATE_H

#include <string>

namespace raft {
    class PersistentState {
    public:
        PersistentState(std::string log_file_path);
    private:
        unsigned int m_current_term = 0;
        unsigned int m_voted_for = 0;
        std::string m_log_file_path = "";
    };
}


#endif //PERSISTENT_STATE_H
