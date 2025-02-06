//
// Created by pknadimp on 2/5/25.
//

#include <gtest/gtest.h>
#include "client.h"
#include "node.h"

class MockClient final : public raft::Client {
public:
    void request_vote(std::string address, int term, int candidate_id, int last_log_index, int last_log_term,
                      std::function<void(int term, bool vote_granted)> callback) override {
        std::thread t([callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(raft::ELECTION_TIMER_MIN_MS / 5));
            // calls the callback with term 0 and vote granted
            callback(0, true);
        });
        t.detach();
    }
};

TEST(ElectionTest, CoreElectionLogic) {
    std::unique_ptr<MockClient> client = std::make_unique<MockClient>();

    raft::ClusterMap cluster_map{
        {1, raft::NodeInfo{"0.0.0.0", 6969}},
        {2, raft::NodeInfo{"0.0.0.0", 7070}},
        {3, raft::NodeInfo{"0.0.0.0", 4206}},
    };

    boost::asio::io_context io_context;
    raft::Node node(1, cluster_map, io_context, std::move(client));

    std::thread stop_thread([&node]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5 * raft::ELECTION_TIMER_MAX_MS));
        node.stop();
    });

    node.run();
    GTEST_ASSERT_EQ(node.get_server_state(), raft::ServerState::LEADER);

    stop_thread.join();
}
