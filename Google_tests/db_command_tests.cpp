//
// Created by pknadimp on 2/15/25.
//

#include <gtest/gtest.h>
#include "command.h"

TEST(CommandSerializationTests, GenericSetCommand) {
    db::Command command {
        .type = db::CommandType::SET,
        .key = "key",
        .value = "10"
    };

    std::string serialization = serialize_command(command);
    GTEST_ASSERT_EQ(serialization, "SET key 10");
}

TEST(CommandDeserializationTests, GenericSetCommand) {
    std::string serialization = "SET key 20";

    std::optional<db::Command> command = db::deserialize_command(serialization);
    GTEST_ASSERT_TRUE(command != std::nullopt);
    GTEST_ASSERT_EQ(command->type, db::CommandType::SET);
    GTEST_ASSERT_EQ(command->key, "key");
    GTEST_ASSERT_EQ(command->value, "20");
}

TEST(CommandDeserializationTests, GenericSetCommandWithNewLine) {
    std::string serialization = "SET key 30\n";

    std::optional<db::Command> command = db::deserialize_command(serialization);
    GTEST_ASSERT_TRUE(command != std::nullopt);
    GTEST_ASSERT_EQ(command->type, db::CommandType::SET);
    GTEST_ASSERT_EQ(command->key, "key");
    GTEST_ASSERT_EQ(command->value, "30");
}