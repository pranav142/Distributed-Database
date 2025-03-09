#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <atomic>
#include <csignal>

#include "utils/logging.h"
#include "utils/cluster.h"
#include "FSM/key_value/db.h"
#include "raft/node.h"
#include "load_balancer/lb.h"
#include "raft/gRPC_client.h"

=
int main() {
    utils::initialize_global_logging();
    std::cout << "Starting distributed database example..." << std::endl;

    utils::ClusterMap cluster_map_1{
        {0, utils::NodeInfo{"127.0.0.1:6969"}},
        {1, utils::NodeInfo{"127.0.0.1:7070"}},
        {2, utils::NodeInfo{"127.0.0.1:4206"}},
    };

    utils::ClusterMap cluster_map_2{
        {3, utils::NodeInfo{"127.0.0.1:6970"}},
        {4, utils::NodeInfo{"127.0.0.1:7071"}},
        {5, utils::NodeInfo{"127.0.0.1:4207"}},
    };

    utils::ClusterMap cluster_map_3{
        {6, utils::NodeInfo{"127.0.0.1:6981"}},
        {7, utils::NodeInfo{"127.0.0.1:7171"}},
        {8, utils::NodeInfo{"127.0.0.1:4217"}},
    };

    utils::Clusters clusters{
        {"Cluster1", cluster_map_1},
        {"Cluster2", cluster_map_2},
        {"Cluster3", cluster_map_3}
    };

    std::vector<std::unique_ptr<raft::Node>> nodes;
    std::vector<std::thread> node_threads;

    auto create_nodes = [&nodes](const utils::ClusterMap& cluster_map) {
        for (auto& [id, node_info] : cluster_map) {
            auto client = std::make_unique<raft::gRPCClient>();
            auto fsm = std::make_shared<kv::DB>();
            auto node = std::make_unique<raft::Node>(id, cluster_map, std::move(client), fsm);
            nodes.push_back(std::move(node));
        }
    };

    create_nodes(cluster_map_1);
    create_nodes(cluster_map_2);
    create_nodes(cluster_map_3);

    std::cout << "Created " << nodes.size() << " nodes across 3 clusters" << std::endl;

    for (size_t i = 0; i < nodes.size(); i++) {
        std::cout << "Starting node " << nodes[i]->get_id() << std::endl;
        node_threads.emplace_back([&, i]() {
            nodes[i]->run();
        });
    }

    std::cout << "Waiting for leader election..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "Starting load balancer on port 8080..." << std::endl;
    const unsigned int replicas = 30;
    const unsigned short port = 8080;

    loadbalancer::LoadBalancer lb(clusters, replicas);
    std::thread lb_thread([&lb, port]() {
        lb.run(port);
    });

    std::cout << "System is running!" << std::endl;
    std::cout << "Access the database at http://localhost:8080/request" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    // Example requests
    std::cout << "\nExample requests:" << std::endl;
    std::cout << "SET: curl -X POST http://localhost:8080/request -H \"Content-Type: application/x-www-form-urlencoded\" -d \"key=mykey&request=SET|mykey|myvalue\"" << std::endl;
    std::cout << "GET: curl -X POST http://localhost:8080/request -H \"Content-Type: application/x-www-form-urlencoded\" -d \"key=mykey&request=GET|mykey|\"" << std::endl;
    std::cout << "DELETE: curl -X POST http://localhost:8080/request -H \"Content-Type: application/x-www-form-urlencoded\" -d \"key=mykey&request=DELETE|mykey|\"" << std::endl;


    if (lb_thread.joinable()) {
        lb_thread.join();
    }

    for (auto& thread : node_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    return 0;
}