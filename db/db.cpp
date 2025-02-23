//
// Created by pknadimp on 2/15/25.
//

#include "db.h"

db::Response db::DB::apply_command(const Command &command) {
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
