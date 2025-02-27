//
// Created by pknadimp on 2/27/25.
//

#include <gtest/gtest.h>
#include <key_value/db.h>

TEST(KvDbTests, SetsValueTest) {
    kv::DB db;

    kv::Request request{
        .type = kv::RequestType::SET,
        .key = "key",
        .value = "10",
    };
    fsm::SerializedData serialized_request = kv::serialize_request(request);
    fsm::FSMResponse fsm_response = db.apply_command(serialized_request);
    GTEST_ASSERT_EQ(fsm_response.error_code, fsm::FSMResponse::ErrorCode::SUCCESS);

    request.type = kv::RequestType::GET;
    request.key = "key";

    serialized_request = kv::serialize_request(request);
    fsm_response = db.query_state(serialized_request);
    GTEST_ASSERT_EQ(fsm_response.error_code, fsm::FSMResponse::ErrorCode::SUCCESS);

    std::optional<kv::Response> response = kv::deserialize_response(fsm_response.serialized_response);
    GTEST_ASSERT_TRUE(response.has_value());
    GTEST_ASSERT_TRUE(response->success);
    GTEST_ASSERT_EQ(response->data, "10");
}

TEST(KvDbTests, ReadsInvalidValueTest) {
    kv::DB db;

    kv::Request request{
        .type = kv::RequestType::GET,
        .key = "key",
    };
    fsm::SerializedData serialized_request = kv::serialize_request(request);
    fsm::FSMResponse fsm_response = db.query_state(serialized_request);
    GTEST_ASSERT_EQ(fsm_response.error_code, fsm::FSMResponse::ErrorCode::FAILED_TO_PROCESS_REQUEST);
}