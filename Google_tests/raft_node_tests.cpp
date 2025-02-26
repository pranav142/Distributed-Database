//
// Created by pknadimp on 2/5/25.
//

#include <gtest/gtest.h>
#include <proto/raft.pb.h>

#include "client.h"
#include "node.h"
#include "gRPC_client.h"
#include "logging.h"
#include "lb.h"
#include "Mock_FSM.h"

class MockClient final : public raft::Client {
public:
    ~MockClient() override = default;

    void request_vote(std::string address, const raft::RequestVoteRPC &request_vote_rpc,
                      std::function<void(raft::RequestVoteResponse)> callback) override {
        std::thread t([callback, request_vote_rpc]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(raft::ELECTION_TIMER_MIN_MS / 5));
            raft::RequestVoteResponse response{};
            response.term = static_cast<int>(request_vote_rpc.term);
            response.vote_granted = true;
            callback(response);
        });
        t.detach();
    }

    void append_entries(std::string address, const raft::AppendEntriesRPC &append_entries,
                        std::function<void(raft::AppendEntriesResponse)> callback) override {
        std::thread t([callback, append_entries]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(raft::ELECTION_TIMER_MIN_MS / 5));
            raft::AppendEntriesResponse response{};
            response.term = append_entries.term;
            response.success = true;
            callback(response);
        });
        t.detach();
    }
};

TEST(ElectionTest, CoreElectionLogic) {
    std::unique_ptr<MockClient> client = std::make_unique<MockClient>();

    utils::ClusterMap cluster_map{
        {0, utils::NodeInfo{"0.0.0.0:6969"}},
        {1, utils::NodeInfo{"0.0.0.0:7070"}},
        {2, utils::NodeInfo{"0.0.0.0:4206"}},
    };

    auto fsm = std::make_shared<MockFSM>();
    raft::Node node(0, cluster_map, std::move(client), fsm);

    std::thread stop_thread([&node]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5 * raft::ELECTION_TIMER_MAX_MS));
        node.cancel();
    });

    node.run();
    GTEST_ASSERT_EQ(node.get_server_state(), raft::ServerState::LEADER);
    stop_thread.join();
}

TEST(ElectionTest, RequestVoteTest) {
    utils::initialize_global_logging();

    raft::gRPCClient client;

    raft::RequestVoteRPC request_vote{};
    request_vote.candidate_id = 2;
    request_vote.last_log_index = 100;
    request_vote.last_log_term = 100000;
    request_vote.term = 100000;

    std::unique_ptr<MockClient> n_client = std::make_unique<MockClient>();


    utils::ClusterMap cluster_map{
        {0, utils::NodeInfo{"127.0.0.1:6969"}},
        {1, utils::NodeInfo{"0.0.0.0:7070"}},
        {2, utils::NodeInfo{"0.0.0.0:4206"}},
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

    auto fsm = std::make_shared<MockFSM>();
    raft::Node node(0, cluster_map, std::move(n_client), std::move(fsm));

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


TEST(ElectionTest, NodeLogReplicationTest) {
    utils::initialize_global_logging();

    auto client1 = std::make_unique<raft::gRPCClient>();
    auto client2 = std::make_unique<raft::gRPCClient>();
    auto client3 = std::make_unique<raft::gRPCClient>();

    utils::ClusterMap cluster_map{
        {0, utils::NodeInfo{"127.0.0.1:6969"}},
        {1, utils::NodeInfo{"127.0.0.1:7070"}},
        {2, utils::NodeInfo{"127.0.0.1:4206"}},
    };

    // this node will time out at the max election time
    auto fsm1 = std::make_shared<MockFSM>();
    raft::TimerSettings timer_settings;
    timer_settings.election_timer_max_ms = raft::ELECTION_TIMER_MAX_MS;
    timer_settings.election_timer_min_ms = raft::ELECTION_TIMER_MAX_MS;

    raft::Node node_1(0, cluster_map, std::move(client1), fsm1, timer_settings);
    node_1.set_current_term(10);

    timer_settings.election_timer_max_ms = raft::ELECTION_TIMER_MIN_MS;
    timer_settings.election_timer_min_ms = raft::ELECTION_TIMER_MIN_MS;

    auto fsm2 = std::make_shared<MockFSM>();
    raft::Node node_2(1, cluster_map, std::move(client2), fsm2, timer_settings);
    node_2.set_current_term(6);


    auto fsm3 = std::make_shared<MockFSM>();
    raft::Node node_3(2, cluster_map, std::move(client3), fsm3, timer_settings);
    node_3.set_current_term(6);

    double average_time_ms = 0.0;
    std::thread stop_thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5 * raft::ELECTION_TIMER_MAX_MS));
        for (int i = 0; i < 10; i++) {
            raft_gRPC::ClientRequest request;
            db::Command command;
            command.key = "x";
            command.type = db::CommandType::SET;
            command.value = std::to_string(i);
            db::serialize_command(command);
            request.set_command(db::serialize_command(command));

            raft_gRPC::ClientResponse response;

            auto start = std::chrono::high_resolution_clock::now();
            bool success = loadbalancer::command_request(cluster_map[0].address, request, &response);
            auto end = std::chrono::high_resolution_clock::now();
            average_time_ms += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        }
        average_time_ms /= 10.0;
        std::this_thread::sleep_for(std::chrono::milliseconds(2 * raft::ELECTION_TIMER_MAX_MS));
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

    std::cout << "Average request time: " << average_time_ms << "ms" << std::endl;

    // Ensure there is only one leader
    GTEST_ASSERT_TRUE(node_1.get_server_state() == raft::ServerState::LEADER);
    GTEST_ASSERT_TRUE(node_2.get_server_state() != raft::ServerState::LEADER);
    GTEST_ASSERT_TRUE(node_3.get_server_state() != raft::ServerState::LEADER);

    // Ensure each node is the same term
    GTEST_ASSERT_TRUE(
        node_1.get_current_term() == node_2.get_current_term() &&
        node_1.get_current_term() == node_3.get_current_term(
        ));

    // fsm1->dump();
}

// This test is for handling the case
// where a node loses communication with other nodes
// and is essentially in a minority parition
TEST(ElectionTest, HandlesOfflineNodesTest) {
    auto client1 = std::make_unique<raft::gRPCClient>();
    auto client2 = std::make_unique<raft::gRPCClient>();
    auto client3 = std::make_unique<raft::gRPCClient>();

    utils::initialize_global_logging();

    utils::ClusterMap cluster_map{
        {0, utils::NodeInfo{"127.0.0.1:6969"}},
        {1, utils::NodeInfo{"127.0.0.1:7070"}},
        {2, utils::NodeInfo{"127.0.0.1:4206"}},
    };

    // this node will time out last but should still end up as the leader
    // by setting the min and max election time to same value
    // we set exactly how frequently the node will be timing out
    auto fsm = std::make_shared<MockFSM>();
    raft::Node node_1(1, cluster_map, std::move(client1), std::move(fsm));
    node_1.set_current_term(10);

    std::thread stop_thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5 * raft::ELECTION_TIMER_MAX_MS));
        node_1.cancel();
    });

    std::thread n1([&node_1]() {
        node_1.run();
    });

    n1.join();
    stop_thread.join();

    // Ensure there is only one leader
    GTEST_ASSERT_TRUE(node_1.get_server_state() != raft::ServerState::LEADER);
}

TEST(ElectionTest, HandlesClientRequest) {
}
