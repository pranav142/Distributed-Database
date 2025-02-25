//
// Created by pknadimp on 2/15/25.
//

#ifndef DB_H
#define DB_H

#include <optional>
#include <unordered_map>
#include <string>
#include <mutex>
#include <utility>

#include "command.h"
#include "consistent_hashing.h"

// TODO: TEST hashing
namespace db {
    // TODO: Rename this to be cleaerer
    struct Response {
        bool success;
        std::string data;
    };

    struct KeyRangeResponse {
        bool success;
        std::vector<std::string> keys;
    };

    class DB {
    public:
        explicit DB(std::function<std::size_t(const std::string&)> hasher = utils::hasher) : m_hasher(std::move(hasher)) {

        };

        ~DB() = default;

        // This is for commands that modify
        // the state of the DB
        Response apply_command(const Command &command);

        // This is for commands that do
        // not modify the state of the DB
        Response query_state(const Command &command);

        static bool is_modifying_command(const Command &command);

        std::optional<std::string> get_value(const std::string &key);

        KeyRangeResponse get_key_range(std::size_t lower_hash, std::size_t upper_hash);

    private:
        // TODO: rename to be clearer
        Response update_db_state(const Command &command);

        void update_key_hash_map(const Command &command);

    private:
        std::mutex m_mtx;
        std::unordered_map<std::string, std::string> m_db;
        std::unordered_map<std::size_t, std::string> m_hash_to_key_map;
        std::function<std::size_t(const std::string&)> m_hasher;
    };
};


#endif //DB_H
