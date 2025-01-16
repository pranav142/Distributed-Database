//
// Created by pknadimp on 1/16/25.
//

#include "persistent_state.h"

raft::PersistentState::PersistentState(std::string log_file_path) : m_log_file_path(std::move(log_file_path)) {

}
