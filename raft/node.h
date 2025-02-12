//
// Created by pknadimp on 1/18/25.
//

#ifndef NODE_H
#define NODE_H

#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "cluster.h"
#include "events.h"
#include "event_queue.h"
#include "persistent_state.h"
#include "client.h"
#include "gRPC_server.h"
#include "timer.h"


namespace raft {
    constexpr int ELECTION_TIMER_MIN_MS = 150;
    constexpr int ELECTION_TIMER_MAX_MS = 300;
    constexpr int HEART_BEAT_INTERVAL_MS = 30;

    enum class ServerState : int {
        FOLLOWER = 0,
        CANDIDATE,
        LEADER,
    };

    class Node {
    public:
        Node(unsigned int id, const ClusterMap &cluster, boost::asio::io_context &io,
             std::unique_ptr<Client> client, unsigned int election_timer_min_ms = ELECTION_TIMER_MIN_MS,
             unsigned int election_timer_max_ms = ELECTION_TIMER_MAX_MS,
             unsigned int heartbeat_interval_ms = HEART_BEAT_INTERVAL_MS) : m_id(id),
                                                                            m_state(
                                                                                "log_" + std::to_string(id) + ".txt"),
                                                                            m_cluster(cluster),
                                                                            m_io(io),
                                                                            m_election_timer(io),
                                                                            m_heartbeat_timer(io),
                                                                            m_strand(make_strand(io)),
                                                                            m_work_guard(
                                                                                make_work_guard(io)),
                                                                            m_client(std::move(client)),
                                                                            m_election_timer_max_ms(
                                                                                election_timer_max_ms),
                                                                            m_election_timer_min_ms(
                                                                                election_timer_min_ms),
                                                                            m_heart_beat_interval_ms(
                                                                                heartbeat_interval_ms),
                                                                            m_logger(spdlog::stdout_color_mt(
                                                                                "node_" + std::to_string(id))) {
        }

        ServerState get_server_state() const;

        unsigned int get_id() const;

        unsigned int get_commit_index() const;

        unsigned int get_last_applied_index() const;

        unsigned int get_current_term() const;

        void set_current_term(unsigned int term);

        void on_election_timeout(const boost::system::error_code &ec);

        void reset_election_timer();

        void on_heartbeat_timeout(const boost::system::error_code &ec);

        void reset_heartbeat_timer();

        void shut_down_heartbeat_timer();

        void log_current_state() const;

        void run();

        void cancel();

    private:
        void initialize();

        void shut_down_election_timer();

        void stop();

        void become_follower(unsigned int term);

        void become_candidate();

        void become_leader();

        void run_follower_loop();

        void request_vote(const std::string &address);

        void start_election();

        void run_candidate_loop();

        bool is_log_more_up_to_date(unsigned int last_log_index, unsigned int last_log_term) const;

        void append_entries(const std::string &address);

        void run_leader_loop();

        int calculate_quorum() const;

        void run_server(const std::string &address);

    private:
        unsigned int m_id;
        unsigned int m_commit_index = 0;
        unsigned int m_last_applied_index = 0;

        ServerState m_server_state = ServerState::FOLLOWER;
        PersistentState m_state;

        // Only used in leader state
        std::vector<unsigned int> m_next_index;
        std::vector<unsigned int> m_match_index;

        // A map of ID's and corresponding IPs
        ClusterMap m_cluster;

        EventQueue<Event> m_event_queue;
        bool m_running = false;

        // Time
        boost::asio::io_context &m_io;
        Timer m_election_timer;
        Timer m_heartbeat_timer;
        unsigned int m_election_timer_min_ms;
        unsigned int m_election_timer_max_ms;
        unsigned int m_heart_beat_interval_ms;

        boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
        // Work guard is needed to keep the io context running even when there is no work
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_work_guard;

        std::unique_ptr<Client> m_client = nullptr;
        std::unique_ptr<RaftSeverImpl> m_service = nullptr;
        std::unique_ptr<grpc::Server> m_server = nullptr;

        std::shared_ptr<spdlog::logger> m_logger;
    };

    std::string server_state_to_str(ServerState state);
}


#endif //NODE_H
