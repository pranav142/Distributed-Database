//
// Created by pknadimp on 1/19/25.
//

#include <gtest/gtest.h>
#include "node.h"

TEST(ElectionTest, SuccessfulCandidateTransition) {
    // This test checks that after one election timeout the node is in the candidate state
    boost::asio::io_context io;
    raft::Node node(1, {}, io);

    std::thread stop_thread([&node]() {
        sleep(raft::ELECTION_TIMER_MAX_MS / 1000);
        node.stop();
    });

    node.run();
    GTEST_ASSERT_EQ(node.get_server_state(), raft::ServerState::CANDIDATE);

    stop_thread.join();
}
