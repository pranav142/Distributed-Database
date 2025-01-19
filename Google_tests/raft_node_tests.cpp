//
// Created by pknadimp on 1/19/25.
//

#include <gtest/gtest.h>
#include "node.h"

TEST(ElectionTest, SuccessfulCandidateTransition) {

    boost::asio::io_context io;
    raft::Node node(1, {}, io);
    node.start();
    io.run();
    // GTEST_ASSERT_EQ(node.get_server_state(), raft::ServerState::FOLLOWER);
}
