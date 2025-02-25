//
// Created by pknadimp on 2/15/25.
//

#include "db.h"

db::Response db::DB::apply_command(const Command &command) {
    Response response = update_db_state(command);
    if (response.success) {
        update_key_hash_map(command);
    }

    return response;
}

db::Response db::DB::query_state(const Command &command) {
    Response response{};
    if (command.type != CommandType::GET) {
        response.success = false;
        response.data = "Command " + command_type_to_str(command.type) + " not supported for non modifying requests";
        return response;
    }

    std::string key = command.key;
    std::optional<std::string> value = get_value(key);

    if (!value.has_value()) {
        response.success = true;
        response.data = "null";
        return response;
    }

    response.success = true;
    response.data = value.value();
    return response;
}

bool db::DB::is_modifying_command(const Command &command) {
    return command.type == CommandType::SET || command.type == CommandType::DELETE;
}

std::optional<std::string> db::DB::get_value(const std::string &key) {
    std::lock_guard lock(m_mtx);

    if (m_db.contains(key)) {
        return m_db[key];
    }
    return std::nullopt;
}

db::KeyRangeResponse db::DB::get_key_range(std::size_t lower_hash, std::size_t upper_hash) {
    KeyRangeResponse response{};
    for (auto& [hash, key] : m_hash_to_key_map) {
        if (hash >= lower_hash && hash <= upper_hash) {
            response.keys.push_back(key);
        }
    }
    response.success = true;
    return response;
}

db::Response db::DB::update_db_state(const Command &command) {
    std::lock_guard lock(m_mtx);

    Response response{};

    if (command.type == CommandType::SET) {
        m_db[command.key] = command.value;
        response.success = true;
        return response;
    }

    if (command.type == CommandType::DELETE) {
        m_db.erase(command.key);
        response.success = true;
        return response;
    }

    response.success = false;
    response.data = "Command " + command_type_to_str(command.type) + " not supported for modifying requests";
    return response;
}

void db::DB::update_key_hash_map(const Command &command) {
    std::size_t hash = m_hasher(command.key);
    if (command.type == CommandType::SET) {
        m_hash_to_key_map[hash] = command.key;
    }

    if (command.type == CommandType::DELETE) {
        m_hash_to_key_map.erase(hash);
    }
}
