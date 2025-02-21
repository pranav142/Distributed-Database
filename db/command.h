//
// Created by pknadimp on 2/15/25.
//

#ifndef COMMAND_H
#define COMMAND_H

#include <optional>
#include <string>

namespace db {
    enum class CommandType {
        GET,
        SET,
        DELETE
    };

    struct Command {
        CommandType type;
        std::string key;
        std::string value;
    };

    std::string serialize_command(const Command &command);

    std::optional<Command> deserialize_command(const std::string &serialized);

    std::optional<std::string> get_key(const std::string &serialized);

    std::optional<CommandType> command_type_from_str(const std::string &str);

    std::string command_type_to_str(CommandType type);
}


#endif //COMMAND_H
