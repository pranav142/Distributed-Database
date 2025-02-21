//
// Created by pknadimp on 2/15/25.
//

#ifndef DB_H
#define DB_H

#include <optional>
#include <unordered_map>
#include <string>
#include <mutex>

#include "command.h"

namespace db {

    struct Response {
        bool success;
        std::string data;
    };

    class DB {
    public:
        DB() = default;

        ~DB() = default;

        // This is for commands that modify
        // the state of the DB
        Response apply_command(const Command &command);

        // This is for commands that do
        // not modify the state of the DB
        Response query_state(const Command &command);

        std::optional<std::string> get_value(const std::string &key);

    private:
        std::mutex m_mtx;
        std::unordered_map<std::string, std::string> m_db;
    };
};


#endif //DB_H
