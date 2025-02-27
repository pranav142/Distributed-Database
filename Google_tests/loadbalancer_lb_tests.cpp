//
// Created by pknadimp on 2/19/25.
//

//#include <gtest/gtest.h>
//#include "logging.h"
//#include "lb.h"
//#include "key_value/command.h"
//#include "cluster.h"
//#include "gRPC_client.h"
//#include "node.h"
//#include "key_value/db.h"
//#include "Mock_FSM.h"
//
//
//std::vector<std::unique_ptr<raft::Node> > create_nodes(const utils::ClusterMap &cluster_map) {
//    std::vector<std::unique_ptr<raft::Node> > nodes;
//
//    for (auto &[id, node_info]: cluster_map) {
//        auto client = std::make_unique<raft::gRPCClient>();
//        auto fsm = std::make_shared<MockFSM>();
//        auto node = std::make_unique<raft::Node>(id, cluster_map, std::move(client), fsm);
//        nodes.push_back(std::move(node));
//    }
//
//    return nodes;
//}
//
//utils::ClusterMap cluster_map_1{
//    {0, utils::NodeInfo{"127.0.0.1:6969"}},
//    {1, utils::NodeInfo{"127.0.0.1:7070"}},
//    {2, utils::NodeInfo{"127.0.0.1:4206"}},
//};
//
//utils::ClusterMap cluster_map_2{
//    {3, utils::NodeInfo{"127.0.0.1:6970"}},
//    {4, utils::NodeInfo{"127.0.0.1:7071"}},
//    {5, utils::NodeInfo{"127.0.0.1:4207"}},
//};
//
//utils::ClusterMap cluster_map_3{
//    {6, utils::NodeInfo{"127.0.0.1:6981"}},
//    {7, utils::NodeInfo{"127.0.0.1:7171"}},
//    {8, utils::NodeInfo{"127.0.0.1:4217"}},
//};
//
//
//utils::Clusters clusters{
//    {"Cluster1", cluster_map_1},
//    {"Cluster2", cluster_map_2},
//    {"Cluster3", cluster_map_3}
//};
//
//TEST(LoadBalancerTests, HandlesRequestRouting) {
//    utils::initialize_global_logging();
//
//
//    loadbalancer::LoadBalancer lb(clusters, db::get_key, 30);
//
//    auto cluster_1 = create_nodes(cluster_map_1);
//    auto cluster_2 = create_nodes(cluster_map_2);
//    auto cluster_3 = create_nodes(cluster_map_3);
//
//    std::vector<std::thread> node_threads;
//
//    auto run_cluster = [&](auto &cluster) {
//        for (auto &node: cluster) {
//            node_threads.emplace_back([&node]() {
//                node->run();
//            });
//        }
//    };
//
//    run_cluster(cluster_1);
//    run_cluster(cluster_2);
//    run_cluster(cluster_3);
//
//    std::thread request_thread([&]() {
//        // correctly sets the values for keys
//        std::this_thread::sleep_for(std::chrono::milliseconds(raft::ELECTION_TIMER_MAX_MS * 10));
//        for (int i = 0; i < 100; i++) {
//            db::Command cmd;
//            cmd.key = "key_" + std::to_string(i);
//            cmd.value = "value_" + std::to_string(i);
//            cmd.type = db::CommandType::SET;
//
//            std::string serialized_command = db::serialize_command(cmd);
//            loadbalancer::LBResponse response = lb.process_request(serialized_command);
//            GTEST_ASSERT_TRUE(response.success);
//        }
//
//        // correctly handles fetching keys
//        // that have been set
//        for (int i = 0; i < 100; i++) {
//            db::Command cmd;
//            cmd.key = "key_" + std::to_string(i);
//            cmd.type = db::CommandType::GET;
//
//            std::string serialized_command = db::serialize_command(cmd);
//            loadbalancer::LBResponse response = lb.process_request(serialized_command);
//            GTEST_ASSERT_TRUE(response.success);
//            GTEST_ASSERT_TRUE(response.data == "value_" + std::to_string(i));
//        }
//
//        // correctly handles fetching keys
//        // that have not been set
//        for (int i = 100; i < 150; i++) {
//             db::Command cmd;
//            cmd.key = "key_" + std::to_string(i);
//            cmd.type = db::CommandType::GET;
//
//            std::string serialized_command = db::serialize_command(cmd);
//            loadbalancer::LBResponse response = lb.process_request(serialized_command);
//            GTEST_ASSERT_TRUE(response.success);
//            GTEST_ASSERT_TRUE(response.data == "null");
//        }
//
//        // deletes keys
//        for (int i = 0; i < 100; i++) {
//            db::Command cmd;
//            cmd.key = "key_" + std::to_string(i);
//            cmd.type = db::CommandType::DELETE;
//
//            std::string serialized_command = db::serialize_command(cmd);
//            loadbalancer::LBResponse response = lb.process_request(serialized_command);
//            GTEST_ASSERT_TRUE(response.success);
//        }
//
//        // ensures keys are deleted
//        for (int i = 0; i < 100; i++) {
//            db::Command cmd;
//            cmd.key = "key_" + std::to_string(i);
//            cmd.type = db::CommandType::GET;
//
//            std::string serialized_command = db::serialize_command(cmd);
//            loadbalancer::LBResponse response = lb.process_request(serialized_command);
//            GTEST_ASSERT_TRUE(response.success);
//            GTEST_ASSERT_TRUE(response.data == "null");
//        }
//
//        std::array clusters_{std::ref(cluster_1), std::ref(cluster_2), std::ref(cluster_3)};
//        for (auto &cluster: clusters_) {
//            for (auto &node: cluster.get()) {
//                node->cancel();
//            }
//        }
//    });
//
//
//    request_thread.join();
//    for (auto &t: node_threads) {
//        if (t.joinable()) {
//            t.join();
//        }
//    }
//}
