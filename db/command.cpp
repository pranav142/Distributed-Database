//
// Created by pknadimp on 2/15/25.
//

#include "command.h"
#include <string>

#include <sstream>

std::string db::command_type_to_str(db::CommandType type) {
    switch (type) {
        case CommandType::GET:
            return "GET";
        case CommandType::DELETE:
            return "DELETE";
        case CommandType::SET:
            return "SET";
        default:
            return "UNKNOWN";
    }
}

std::optional<db::CommandType> db::command_type_from_str(const std::string &str) {
    switch (str[0]) {
        case 'G':
            return CommandType::GET;
        case 'D':
            return CommandType::DELETE;
        case 'S':
            return CommandType::SET;
        default:
            return std::nullopt;
    }
}

std::string db::serialize_command(const Command &command) {
    std::stringstream ss;
    ss << command_type_to_str(command.type) << " " << command.key << " ";
    if (command.value.empty()) {
        ss << "null";
    } else {
        ss << command.value;
    }
    return ss.str();
}

std::optional<db::Command> db::deserialize_command(const std::string &serialized) {
    std::istringstream ss(serialized);
    std::string type_str, key_str, value_str;

    if (!std::getline(ss, type_str, ' ') || !std::getline(ss, key_str, ' ') || !std::getline(ss, value_str)) {
        return std::nullopt;
    }

    std::optional<db::CommandType> type = command_type_from_str(type_str);
    if (!type.has_value()) {
        return std::nullopt;
    }

    if (value_str == "null") {
        value_str = "";
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
