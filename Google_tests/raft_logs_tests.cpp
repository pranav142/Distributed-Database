//
// Created by pknadimp on 1/16/25.
//

#include <gtest/gtest.h>
#include "logs.h"
#include <optional>

TEST(SerializeLogTest, HandlesValidLog) {
    raft::Log log(1, 1, "x->3");
    std::string serialized_log = serialize_log(log);

    GTEST_ASSERT_TRUE(serialized_log == "1,1,x->3\n");
}

TEST(DeserializeLogTest, HandlesValidSerialization) {
    std::string serialized_log = "2,3,x->4\n";
    raft::Log log = raft::deserialize_log(serialized_log).value();

    GTEST_ASSERT_TRUE(log.index == 2);
    GTEST_ASSERT_TRUE(log.term == 3);
    GTEST_ASSERT_TRUE(log.entry == "x->4");
}

TEST(DeserializeLogTest, HandlesValidSerializationWithoutNewLine) {
    std::string serialized_log = "2,3,x->4";
    raft::Log log = raft::deserialize_log(serialized_log).value();

    GTEST_ASSERT_TRUE(log.index == 2);
    GTEST_ASSERT_TRUE(log.term == 3);
    GTEST_ASSERT_TRUE(log.entry == "x->4");
}

TEST(DeserializeLogTest, HandlesMissingSerializationValues) {
    std::string serialized_log = "2,3\n";
    auto log = raft::deserialize_log(serialized_log);

    GTEST_ASSERT_TRUE(log == std::nullopt);
}

TEST(DeserializeLogTest, HandleInvalidSerializationValues) {
    std::string serialized_log = "-1,1,x->4\n";
    auto log = raft::deserialize_log(serialized_log);

    GTEST_ASSERT_TRUE(log == std::nullopt);
}
