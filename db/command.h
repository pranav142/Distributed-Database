//
// Created by pknadimp on 2/15/25.
//

#ifndef COMMAND_H
#define COMMAND_H

#include <optional>
#include <string>

namespace db {
    enum class CommandType {
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
}


#endif //COMMAND_H
