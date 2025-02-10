//
// Created by pknadimp on 2/5/25.
//

#include <gtest/gtest.h>
#include <proto/raft.pb.h>

#include "client.h"
#include "node.h"
#include "gRPC_client.h"

class MockClient final : public raft::Client {
public:
    ~MockClient() override = default;

    void request_vote(std::string address, const raft::RequestVoteRPC &request_vote_rpc,
                      std::function<void(raft::RequestVoteResponse)> callback) override {
        std::thread t([callback, request_vote_rpc, address]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(raft::ELECTION_TIMER_MIN_MS / 5));
            raft::RequestVoteResponse response{};
            response.term = static_cast<int>(request_vote_rpc.term);
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
        node.cancel();
    });

    node.run();
    GTEST_ASSERT_EQ(node.get_server_state(), raft::ServerState::LEADER);

    stop_thread.join();
}

TEST(ElectionTest, RequestVoteTest) {
    raft::gRPCClient client;

    raft::RequestVoteRPC request_vote{};
    request_vote.candidate_id = 2;
    request_vote.last_log_index = 100;
    request_vote.last_log_term = 100000;
    request_vote.term = 100000;

    std::unique_ptr<MockClient> n_client = std::make_unique<MockClient>();

    raft::ClusterMap cluster_map{
        {1, raft::NodeInfo{"127.0.0.1:6969"}},
        {2, raft::NodeInfo{"0.0.0.0:7070"}},
        {3, raft::NodeInfo{"0.0.0.0:4206"}},
    };


    bool success;
    std::thread request_thread([&]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "requesting vote: " << cluster_map[1].address << std::endl;
        client.request_vote(cluster_map[1].address, request_vote,
                            [&success](const raft::RequestVoteResponse &response) {
                                success = response.term != -1;
                            });
    });

    boost::asio::io_context io_context;
    raft::Node node(1, cluster_map, io_context, std::move(n_client));

    std::thread stop_thread([&node]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5 * raft::ELECTION_TIMER_MAX_MS));
        node.cancel();
    });

    node.run();

    request_thread.join();
    //     GTEST_ASSERT_EQ(node.get_server_state(), raft::ServerState::LEADER);
    //
    stop_thread.join();
    GTEST_ASSERT_TRUE(success);
}

TEST(ElectionTest, NodeElectionTest) {
    auto client1 = std::make_unique<raft::gRPCClient>();
    auto client2 = std::make_unique<raft::gRPCClient>();
    auto client3 = std::make_unique<raft::gRPCClient>();

    raft::ClusterMap cluster_map{
        {1, raft::NodeInfo{"127.0.0.1:6969"}},
        {2, raft::NodeInfo{"127.0.0.1:7070"}},
        {3, raft::NodeInfo{"127.0.0.1:4206"}},
    };

    boost::asio::io_context ctx1;
    raft::Node node_1(1, cluster_map, ctx1, std::move(client1));

    boost::asio::io_context ctx2;
    raft::Node node_2(2, cluster_map, ctx2, std::move(client2));

    boost::asio::io_context ctx3;
    raft::Node node_3(3, cluster_map, ctx3, std::move(client3));


   std::thread stop_thread([&]() {
       std::this_thread::sleep_for(std::chrono::milliseconds(10 * raft::ELECTION_TIMER_MAX_MS));
       node_1.cancel();
       node_2.cancel();
       node_3.cancel();
   });

    std::thread n1([&node_1]() {
        node_1.run();
    });

    std::thread n2([&node_2]() {
        node_2.run();
    });

    std::thread n3([&node_3]() {
        node_3.run();
    });

    n1.join();
    n2.join();
    n3.join();
    stop_thread.join();
}
