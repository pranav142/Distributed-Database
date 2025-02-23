//
// Created by pknadimp on 2/14/25.
//

#ifndef LB_H
#define LB_H

#include <proto/raft.grpc.pb.h>
#include <string>
#include <utility>
#include "cluster.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "consistent_hashing.h"
#include "logging.h"

namespace loadbalancer {
    struct LBResponse {
        bool success;
        std::string data;
    };

    class LoadBalancer {
    public:
        explicit LoadBalancer(utils::Clusters clusters,
                              std::function<std::optional<std::string>(const std::string &)> get_key,
                              unsigned int replicas) : m_clusters(std::move(clusters)),
                                                       m_logger(spdlog::stdout_color_mt("load balancer")),
                                                       m_get_key(std::move(get_key)),
                                                       m_consistent_hash(replicas, hasher) {
            initialize_clusters();
        }

        ~LoadBalancer() = default;

        loadbalancer::LBResponse process_request(const std::string &serialized_command);

    private:
        static std::size_t hasher(const std::string &s);

        void initialize_clusters();

        void update_leader_cache(const std::string &shard, const std::string &leader_address);

        std::optional<std::string> get_cached_leader(const std::string &shard);

        bool send_request_to_leader(const std::string &shard,
                                    const raft_gRPC::ClientRequest &request,
                                    raft_gRPC::ClientResponse *response);

    private:
        utils::Clusters m_clusters;
        std::shared_ptr<spdlog::logger> m_logger;

        std::function<std::optional<std::string>(const std::string &s)> m_get_key;
        utils::ConsistentHashing m_consistent_hash;

        std::unordered_map<std::string, std::string> m_leader_cache;
    };

    bool command_request(const std::string &address,
                         const raft_gRPC::ClientRequest &request,
                         raft_gRPC::ClientResponse *response);
}
#endif //LB_H
