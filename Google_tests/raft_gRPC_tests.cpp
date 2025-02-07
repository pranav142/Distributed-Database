//
// Created by pknadimp on 2/6/25.
//

#include <gtest/gtest.h>
#include "gRPC_client.h"

TEST(gRPCTests, gRPCClientTest) {
    raft::gRPCClient client;
    std::string server_address = "127.0.0.1:6969";

    raft::RequestVoteRPC request_vote{};
    request_vote.candidate_id = 1;
    request_vote.last_log_index = 0;
    request_vote.last_log_term = 0;
    request_vote.term = 0;

    bool success;
    client.request_vote(server_address, request_vote, [&success](const raft::RequestVoteResponse& response) {
        success = response.success;
    });

    GTEST_ASSERT_TRUE(success);
}
