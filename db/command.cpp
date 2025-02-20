//
// Created by pknadimp on 2/15/25.
//

#include "command.h"
#include <string>

#include <sstream>

std::string command_type_to_str(db::CommandType type) {
    switch (type) {
        case db::CommandType::SET:
            return "SET";
        case db::CommandType::DELETE:
            return "DELETE";
        default:
            return "UNKNOWN";
    }
}

std::optional<db::CommandType> command_type_from_str(const std::string &str) {
    switch (str[0]) {
        case 'S':
            return db::CommandType::SET;
        case 'D':
            return db::CommandType::DELETE;
        default:
            return std::nullopt;
    }
}

std::string db::serialize_command(const Command &command) {
    std::stringstream ss;
    ss << command_type_to_str(command.type) << " " << command.key << " " << command.value;
    return ss.str();
}

std::optional<db::Command> db::deserialize_command(const std::string &serialized) {
    std::istringstream ss(serialized);
    std::string type_str, key_str, value_str;

    if (!std::getline(ss, type_str, ' ') || !std::getline(ss, key_str, ' ') || !std::getline(ss, value_str)) {
        return std::nullopt;
    }

    std::optional<db::CommandType> type = command_type_from_str(type_str);
    if (type == std::nullopt) {
        return std::nullopt;
    }

    return Command{type.value(), key_str, value_str};
}

std::optional<std::string> db::get_key(const std::string &serialized) {
    auto cmd = db::deserialize_command(serialized);
    if (cmd == std::nullopt) {
        return std::nullopt;
    }
    return cmd.value().key;
}
