//
// Created by pknadimp on 2/15/25.
//

#include <gtest/gtest.h>
#include "db.h"

TEST(DbTests, SetValueTest) {
    db::DB db;
    db::Command command{
        .type = db::CommandType::SET,
        .key = "key",
        .value = "10",
    };

    auto response = db.apply_command(command);
    GTEST_ASSERT_TRUE(response.success);
    GTEST_ASSERT_TRUE(db.get_value("key") == "10");
}

TEST(DbTests, DeleteValueTest) {
    db::DB db;
    db::Command command{
        .type = db::CommandType::SET,
        .key = "key",
        .value = "10",
    };

    auto response = db.apply_command(command);
    GTEST_ASSERT_TRUE(response.success);

    command.type = db::CommandType::DELETE;
    command.key = "key";

    response = db.apply_command(command);
    GTEST_ASSERT_TRUE(response.success);
    GTEST_ASSERT_TRUE(db.get_value("key") == std::nullopt);
}

TEST(DbTests, GetNonExistingValues) {
    db::DB db;

    auto value = db.get_value("key");
    GTEST_ASSERT_TRUE(value == std::nullopt);
}

TEST(DbTests, QueryValidStateCommand) {
    db::DB db;

    db::Command command{
        .type = db::CommandType::SET,
        .key = "key",
        .value = "10",
    };

    auto response = db.apply_command(command);
    GTEST_ASSERT_TRUE(response.success);

    command.type = db::CommandType::GET;
    command.key = "key";

    response = db.query_state(command);
    GTEST_ASSERT_TRUE(response.success);
    GTEST_ASSERT_TRUE(response.data == "10");
}
