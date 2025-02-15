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

    class DB {
    public:
        DB() = default;

        ~DB() = default;

        void apply_command(const Command &command);

        std::optional<std::string> get_value(const std::string &key);

    private:
        std::mutex m_mtx;
        std::unordered_map<std::string, std::string> m_db;
    };
};


#endif //DB_H
