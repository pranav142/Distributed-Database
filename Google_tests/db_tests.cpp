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

    db.apply_command(command);
    GTEST_ASSERT_TRUE(db.get_value("key") == "10");
}

TEST(DbTests, DeleteValueTest) {
    db::DB db;
    db::Command command{
        .type = db::CommandType::SET,
        .key = "key",
        .value = "10",
    };

    db.apply_command(command);

    command.type = db::CommandType::DELETE;
    command.key = "key";

    db.apply_command(command);

    GTEST_ASSERT_TRUE(db.get_value("key") == std::nullopt);
}

TEST(DbTests, GetNonExistingValues) {
    db::DB db;

    auto value = db.get_value("key");
    GTEST_ASSERT_TRUE(value == std::nullopt);
}
