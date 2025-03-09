//
// Created by pknadimp on 2/14/25.
//

#include "lb.h"
#include "gRPC_client.h"

void loadbalancer::LoadBalancer::run(unsigned short http_port) {
    m_is_running = true;

    std::thread server_thread([this, http_port]() { m_http_server.run(http_port); });

    std::thread event_thread([this]() {
        while (m_is_running) {
            RequestEvent event = m_http_request_queue.pop();
            std::visit([this](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, HTTPRequestEvent>) {
                    LBClientResponse response = process_request(arg.request);
                    arg.callback(response);
                } else if constexpr (std::is_same_v<T, QuitEvent>) {
                    stop();
                }
            }, event);
        }
    });

    if (event_thread.joinable()) {
        event_thread.join();
    }

    if (server_thread.joinable()) {
        server_thread.join();
    }
}

void loadbalancer::LoadBalancer::shutdown() {
    m_http_server.shutdown();

    m_http_request_queue.push(QuitEvent{});
}

void loadbalancer::LoadBalancer::stop() {
    m_is_running = false;
}

loadbalancer::LBClientResponse loadbalancer::LoadBalancer::process_request(const LBClientRequest &request) {
    raft_gRPC::ClientRequest gRPC_request;
    raft_gRPC::ClientResponse gRPC_response;
    LBClientResponse lb_response;

    constexpr unsigned int MAX_TRIES = 3;
    constexpr unsigned int NEXT_ATTEMPT_DELAY_MS = 100;

    std::optional<std::string> cluster_name = m_consistent_hash.get_node(request.key);
    if (cluster_name == std::nullopt) {
        m_logger->warn("Could not find a cluster to allocate key");
        lb_response.error_code = LBClientResponse::ErrorCode::COULD_NOT_FIND_VALID_CLUSTER;
        return lb_response;
    }
    m_logger->debug("Sending request (key: {}) to Cluster: {}", request.key, cluster_name.value());

    gRPC_request.set_command(utils::serialized_data_to_string(request.request));

    for (int i = 0; i < MAX_TRIES; i++) {
        if (send_request_to_leader(cluster_name.value(), gRPC_request, &gRPC_response)) {
            lb_response.error_code = LBClientResponse::ErrorCode::SUCCESS;
            lb_response.response = utils::serialized_data_from_string(gRPC_response.data());
            return lb_response;
        }
        m_logger->warn("Failed to send request to leader trying again");
        std::this_thread::sleep_for(std::chrono::milliseconds(NEXT_ATTEMPT_DELAY_MS));
    }

    lb_response.error_code = LBClientResponse::ErrorCode::INVALID_LEADER;
    lb_response.response = utils::serialized_data_from_string(gRPC_response.data());
    return lb_response;
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
    bool success;

    if (!leader_address.has_value()) {
        for (const auto &[id, node_info]: cluster_map) {
            success = send_request_grpc(node_info.address, request, response);
            if (!success) {
                continue;
            }

            if (response->success()) {
                update_leader_cache(shard, cluster_map.at(id).address);
                return true;
            }

            if (response->redirect()) {
                leader_address = cluster_map.at(response->leader_id()).address;
                update_leader_cache(shard, cluster_map.at(response->leader_id()).address);
                break;
            }
        }
    }

    if (!leader_address.has_value()) {
        return false;
    }

    success = send_request_grpc(leader_address.value(), request, response);
    if (!success || !response->success()) {
        m_leader_cache.erase(shard);
        return false;
    }

    return true;
}
