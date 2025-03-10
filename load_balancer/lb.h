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
#include "event_queue.h"
#include "event.h"
#include "http_server.h"

// TODO: Bring in the HTTP server
// TODO: Work on implementing request handling
namespace loadbalancer {

    class LoadBalancer {
    public:
        explicit LoadBalancer(utils::Clusters clusters,
                              unsigned int replicas) : m_clusters(std::move(clusters)),
                                                       m_logger(spdlog::stdout_color_mt("load balancer")),
                                                       m_consistent_hash(replicas, utils::hasher) {
            initialize_clusters();
        }

        ~LoadBalancer() = default;

        void run(unsigned short http_port);

        void shutdown();

        LBClientResponse process_request(const LBClientRequest &request);
    private:
        void stop();

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

        utils::ConsistentHashing m_consistent_hash;
        std::unordered_map<std::string, std::string> m_leader_cache;

        utils::EventQueue<RequestEvent> m_http_request_queue;

        HTTPServer m_http_server = HTTPServer(m_http_request_queue);
        bool m_is_running = false;
    };
}
#endif //LB_H
