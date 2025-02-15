//
// Created by pknadimp on 2/15/25.
//

#include "db.h"

void db::DB::apply_command(const Command &command) {
    std::lock_guard lock(m_mtx);

    if (command.type == CommandType::SET) {
        m_db[command.key] = command.value;
    } else if (command.type == CommandType::DELETE) {
        m_db.erase(command.key);
    }
}

std::optional<std::string> db::DB::get_value(const std::string &key) {
    std::lock_guard lock(m_mtx);
    if (m_db.contains(key)) {
        return m_db[key];
    }
    return std::nullopt;
}
