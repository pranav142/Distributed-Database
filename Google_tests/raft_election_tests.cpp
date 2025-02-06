//
// Created by pknadimp on 2/5/25.
//

#include <gtest/gtest.h>
#include <proto/raft.pb.h>

#include "client.h"
#include "node.h"

// TODO: FIX for new interface
class MockClient final : public raft::Client {
public:
    void request_vote(std::string address, const raft::RequestVoteRPC &request_vote_rpc,
                      std::function<void(raft::RequestVoteResponse)> callback) override {
        std::thread t([callback, request_vote_rpc, address]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(raft::ELECTION_TIMER_MIN_MS / 5));
            raft::RequestVoteResponse response;
            response.term = static_cast<int>(request_vote_rpc.term);
            response.address = address;
            response.success = true;
            response.vote_granted = true;
            callback(response);
        });
        t.detach();
    }
};

TEST(ElectionTest, CoreElectionLogic) {
    std::unique_ptr<MockClient> client = std::make_unique<MockClient>();

    raft::ClusterMap cluster_map{
        {1, raft::NodeInfo{"0.0.0.0:6969"}},
        {2, raft::NodeInfo{"0.0.0.0:7070"}},
        {3, raft::NodeInfo{"0.0.0.0:4206"}},
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
