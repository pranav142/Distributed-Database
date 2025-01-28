//
// Created by pknadimp on 1/19/25.
//

#include <httplib.h>
#include <gtest/gtest.h>

#include "node.h"
#include "client.h"

TEST(ElectionTest, SuccessfulCandidateTransition) {
    // This test checks that after election timeouts the node is in the candidate state
    boost::asio::io_context io;

    raft::Node node(1, {}, io, nullptr);

    // Stops the node after 5 times the maximum election timeout
    // This should mean there are 5 election timeout events
    std::thread stop_thread([&node]() {
        sleep(5 * raft::ELECTION_TIMER_MAX_MS / 1000);
        node.stop();
    });

    node.run();
    GTEST_ASSERT_EQ(node.get_server_state(), raft::ServerState::CANDIDATE);

    stop_thread.join();
}
