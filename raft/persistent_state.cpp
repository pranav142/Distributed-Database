//
// Created by pknadimp on 1/16/25.
//

#include "persistent_state.h"
#include <fstream>
#include <gtest/gtest.h>
#include "error_codes.h"
#include "utils.h"

raft::PersistentState::PersistentState(std::string log_file_path) : m_log_file_path(std::move(log_file_path)) {
    if (load_state() == SUCCESS) {
        return;
    }

    // If unable to load state from the file we write to the file state with initial values
    clear_file(log_file_path);
    if (save_state() != SUCCESS) {
        std::cerr << "Failed to save persistent state" << std::endl;
        return;
    }
}

// Log indices start at 1
unsigned int raft::PersistentState::get_last_log_index() const {
    std::string last_line, index_str;

    last_line = get_last_line(m_log_file_path);
    std::istringstream iss(last_line);
    std::getline(iss, index_str, ',');

    // If no logs have been written return a index of 0
    if (index_str == "Header") {
        return 0;
    }

    return std::stoi(index_str);
}

unsigned int raft::PersistentState::get_last_log_term() const {
    std::string last_line, index_str, term_str;

    last_line = get_last_line(m_log_file_path);
    std::istringstream iss(last_line);
    std::getline(iss, index_str, ',');
    std::getline(iss, term_str, ',');

    // If no logs have been written return a log index of 0
    if (index_str == "Header") {
        return 0;
    }

    return std::stoi(term_str);

}

raft::ErrorCode raft::PersistentState::append_log(const std::string &entry) const {
    // The next log index will be one more than the last log index
    unsigned int index = get_last_log_index() + 1;

    Log log(index, m_current_term, entry);
    return append_to_file(m_log_file_path, serialize_log(log));
}

// Index starts at 1
std::optional<raft::Log> raft::PersistentState::read_log(unsigned int index) const {
    if (get_last_log_index() < index || index <= 0) {
        return std::nullopt;
    }

    std::string line;
    std::ifstream file(m_log_file_path);

    while (std::getline(file, line)) {
        auto log = deserialize_log(line);
        // Skip logs that can't be deserialized
        if (log == std::nullopt) {
            continue;
        }

        if (log.value().index == index) {
            return log;
        }
    }

    return std::nullopt;
}

int raft::PersistentState::get_voted_for() const {
    return m_voted_for;
}

void raft::PersistentState::set_voted_for(int candidate_id) {
    m_voted_for = candidate_id;
    if (save_state() != SUCCESS) {
        std::cerr << "Failed to save persistent state" << std::endl;
    }
}

unsigned int raft::PersistentState::get_current_term() const {
    return m_current_term;
}

void raft::PersistentState::set_current_term(unsigned int term) {
    m_current_term = term;
    if (save_state() != SUCCESS) {
        std::cerr << "Failed to save persistent state" << std::endl;
    }
}

void raft::PersistentState::increment_term() {
    set_current_term(get_current_term() + 1);
}

raft::ErrorCode raft::PersistentState::save_state() const {
    std::stringstream ss;
    ss << "Header," << m_current_term << "," << m_voted_for << "\n";
    return replace_line(m_log_file_path, 1, ss.str());
}

raft::ErrorCode raft::PersistentState::load_state() {
    std::ifstream file(m_log_file_path);
    if (!file.is_open()) {
        return FILE_NOT_FOUND;
    }

    std::string metadata_header;
    std::string current_term_str, voted_for_str, header_str;

    std::getline(file, metadata_header);
    std::istringstream metadata_stream(metadata_header);

    if (!std::getline(metadata_stream, header_str, ',') || !std::getline(metadata_stream, current_term_str, ',') || !std::getline(metadata_stream, voted_for_str, '\n')) {
        return FAILED_TO_PARSE_HEADER;
    }

    if (header_str != HEADER_INDICATOR) {
        return INVALID_HEADER_INFO;
    }

    try {
        m_current_term = std::stoi(current_term_str);
        m_voted_for = std::stoi(voted_for_str);
    } catch (const std::exception &e) {
        return INVALID_HEADER_INFO;
    }

    return SUCCESS;
}

bool raft::PersistentState::has_voted_for_no_one() const {
    return m_voted_for == -1;
}

void raft::PersistentState::set_vote_for_no_one()  {
    m_voted_for = -1;
}
