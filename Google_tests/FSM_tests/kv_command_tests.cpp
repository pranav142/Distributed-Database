//
// Created by pknadimp on 2/27/25.
//

#include <gtest/gtest.h>
#include <key_value/command.h>

TEST(CommandSerializationTest, CommandSerialization) {
    kv::Request request{
        .type = kv::RequestType::GET,
        .key = "hello",
        .value = "world",
    };

    fsm::SerializedData data = kv::serialize_request(request);
    std::optional<kv::Request> serialized_data = kv::deserialize_request(data);

    GTEST_ASSERT_TRUE(serialized_data.has_value());

    GTEST_ASSERT_EQ(serialized_data->type, kv::RequestType::GET);
    GTEST_ASSERT_EQ(serialized_data->key, "hello");
    GTEST_ASSERT_EQ(serialized_data->value, "world");
}
