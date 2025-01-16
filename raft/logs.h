//
// Created by pknadimp on 1/16/25.
//

#ifndef LOGS_H
#define LOGS_H

#include <string>
#include <optional>
#include <sstream>


namespace raft {
    struct Log {
        unsigned int index;
        unsigned int term;
        std::string entry;

        Log(const unsigned int &index, const unsigned int &term, std::string entry) : index(index), term(term),
            entry(std::move(entry)) {
        }

        friend std::ostream &operator<<(std::ostream &os, const Log &log) {
            os << "Index: " << log.index << " , Term: " << log.term << " , Entry: " << log.entry;
            return os;
        }
    };

    std::string serialize_log(const Log &log);
    std::optional<Log> deserialize_log(const std::string &serialized_log);
}

#endif //LOGS_H
