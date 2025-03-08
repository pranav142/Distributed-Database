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
#include "serialized_data.h"

namespace loadbalancer {
    struct LBClientRequest {
        // Key for request routing
        std::string key;
        util::SerializedData request;
    };

    struct LBClientResponse {
        enum class ErrorCode {
            SUCCESS,
            COULD_NOT_FIND_VALID_CLUSTER,
            INVALID_LEADER,
        } error_code;
        util::SerializedData response;
    };

    struct LBResponse {
        bool success;
        std::string data;
    };

    class LoadBalancer {
    public:
        explicit LoadBalancer(utils::Clusters clusters,
                              unsigned int replicas) : m_clusters(std::move(clusters)),
                                                       m_logger(spdlog::stdout_color_mt("load balancer")),
                                                       m_consistent_hash(replicas, utils::hasher) {
            initialize_clusters();
        }

        ~LoadBalancer() = default;

        LBClientResponse process_request(const LBClientRequest &request);

    private:
        void initialize_clusters();

        void update_leader_cache(const std::string &shard, const std::string &leader_address);

        std::optional<std::string> get_cached_leader(const std::string &shard);

        bool send_request_to_leader(const std::string &shard,
                                    const raft_gRPC::ClientRequest &request,
                                    raft_gRPC::ClientResponse *response);

    private:
        // cluster manager should store cluster mappings and
        utils::Clusters m_clusters;
        std::shared_ptr<spdlog::logger> m_logger;

        std::function<std::optional<std::string>(const std::string &s)> m_get_key;
        utils::ConsistentHashing m_consistent_hash;

        std::unordered_map<std::string, std::string> m_leader_cache;
    };
}
#endif //LB_H
