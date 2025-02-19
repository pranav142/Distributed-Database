//
// Created by pknadimp on 2/14/25.
//

#include "lb.h"

#include <boost/system/error_code.hpp>
#include <grpcpp/grpcpp.h>
#include <oneapi/tbb/partitioner.h>

bool loadbalancer::LoadBalancer::process_request(const std::string &serialized_command) {
    raft_gRPC::ClientRequest request;
    raft_gRPC::ClientResponse response;
    constexpr unsigned int MAX_TRIES = 3;
    constexpr unsigned int NEXT_ATTEMPT_DELAY_MS = 100;

    std::optional<std::string> key = m_get_key(serialized_command);
    if (key == std::nullopt) {
        m_logger->warn("Could not parse key from command {}", serialized_command);
        return false;
    }

    std::optional<std::string> cluster_name = m_consistent_hash.get_node(key.value());
    if (cluster_name == std::nullopt) {
        m_logger->warn("Could not find a node to allocate key to");
        return false;
    }

    request.set_command(serialized_command);

    for (int i = 0; i < MAX_TRIES; i++) {
        if (send_request_to_leader(cluster_name.value(), request, &response))
            return true;
        // wait till we send another request so
        // the raft cluster can converge
        std::this_thread::sleep_for(std::chrono::milliseconds(NEXT_ATTEMPT_DELAY_MS));
    }

    return false;
}

std::size_t loadbalancer::LoadBalancer::hasher(const std::string &s) {
    std::hash<std::string> hash;
    return hash(s);
}

void loadbalancer::LoadBalancer::initialize_clusters() {
    for (auto &[cluster_name, _]: m_clusters) {
        m_consistent_hash.add_node(cluster_name);
    }
}

void loadbalancer::LoadBalancer::update_leader_cache(const std::string &shard, const std::string &leader_address) {
    m_leader_cache[shard] = leader_address;
}

std::optional<std::string> loadbalancer::LoadBalancer::get_cached_leader(const std::string &shard) {
    if (m_leader_cache.contains(shard)) {
        return m_leader_cache[shard];
    }
    return std::nullopt;
}

bool loadbalancer::LoadBalancer::send_request_to_leader(const std::string &shard,
                                                        const raft_gRPC::ClientRequest &request,
                                                        raft_gRPC::ClientResponse *response) {
    utils::ClusterMap cluster_map = m_clusters[shard];
    std::optional<std::string> leader_address = get_cached_leader(shard);

    if (!leader_address.has_value()) {
        for (const auto &[id, node_info]: cluster_map) {
            if (command_request(node_info.address, request, response)) {
                if (response->redirect()) {
                    leader_address = cluster_map.at(response->leader_id()).address;
                } else {
                    leader_address = node_info.address;
                }
                update_leader_cache(shard, leader_address.value());
                break;
            }
        }
    }

    if (leader_address.has_value()) {
        bool success = command_request(leader_address.value(), request, response);
        if (!success) {
            m_leader_cache.erase(shard);
            return false;
        }

        if (response->redirect()) {
            std::string new_leader = cluster_map.at(response->leader_id()).address;
            update_leader_cache(shard, new_leader);
            success = command_request(new_leader, request, response);
            return success;
        }
        return success;
    }

    return false;
}


bool loadbalancer::command_request(const std::string &address, const raft_gRPC::ClientRequest &request,
                                   raft_gRPC::ClientResponse *response) {
    grpc::ClientContext context;
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    auto stub = raft_gRPC::RaftService::NewStub(channel);
    grpc::Status status = stub->HandleClientRequest(&context, request, response);
    return status.ok();
}
