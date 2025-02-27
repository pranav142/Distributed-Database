//
// Created by pknadimp on 2/15/25.
//

#include <absl/strings/str_format.h>
#include <gtest/gtest.h>
#include "key_value/db.h"

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

std::size_t custom_hasher(const std::string &key) {
    std::size_t pos = key.find_last_of('_');
    if (pos != std::string::npos) {
        return std::stoi(key.substr(pos + 1));
    }

    return 0;
}

TEST(DbTests, QueryHashRange) {
    db::DB db = db::DB(custom_hasher);

    for (int i = 0; i < 10; i++) {
        db::Command command{
            .type = db::CommandType::SET,
            .key = "key_" + std::to_string(i),
            .value = "10",
        };

        auto response = db.apply_command(command);
        GTEST_ASSERT_TRUE(response.success);
    }

    std::size_t lower_hash = custom_hasher("key_5");
    std::size_t upper_hash = custom_hasher("key_8");
    auto response = db.get_key_range(lower_hash, upper_hash);

    std::array needed_keys = {"key_5", "key_6", "key_7", "key_8"};
    GTEST_ASSERT_TRUE(response.keys.size() == needed_keys.size());

    for (auto& key : response.keys) {
        bool success;
         for (auto& needed_key : needed_keys) {
            if (key == needed_key) {
                success = true;
                break;
            }
        }
        GTEST_ASSERT_TRUE(success);
    }
}
