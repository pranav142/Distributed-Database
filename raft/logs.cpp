//
// Created by pknadimp on 1/16/25.
//

#include "logs.h"

std::optional<raft::Log> raft::deserialize_log(const std::string &serialized_log) {
    std::istringstream ss(serialized_log);
    std::string index_str, term_str, entry;
    int index, term;

    if (!std::getline(ss, index_str, ',') ||
        !std::getline(ss, term_str, ',') ||
        !std::getline(ss, entry, '\n')) {
        return std::nullopt;
    }

    try {
        index = std::stoi(index_str);
        term = std::stoi(term_str);
    } catch (const std::exception &e) {
        return std::nullopt;
    }

    if (index < 0 || term < 0) {
        return std::nullopt;
    }

    return Log(index, term, entry);
}

std::string raft::serialize_log(const Log &log) {
    std::stringstream ss;
    ss << log.index << "," << log.term << "," << log.entry << "\n";
    return ss.str();
}
