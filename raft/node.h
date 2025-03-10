//
// Created by pknadimp on 1/18/25.
//

#ifndef NODE_H
#define NODE_H

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <vector>

#include "FSM.h"
#include "client.h"
#include "cluster.h"
#include "event_queue.h"
#include "events.h"
#include "gRPC_server.h"
#include "persistent_state.h"
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

    struct TimerSettings {
        unsigned int election_timer_min_ms = ELECTION_TIMER_MIN_MS;
        unsigned int election_timer_max_ms = ELECTION_TIMER_MAX_MS;
        unsigned int heart_beat_interval_ms = HEART_BEAT_INTERVAL_MS;
        unsigned int lease_timer_ms = HEART_BEAT_INTERVAL_MS * 2;
    };

    class Node {
    public:
        Node(unsigned int id, const utils::ClusterMap &cluster,
             std::unique_ptr<Client> client, std::shared_ptr<fsm::FSM> fsm,
             TimerSettings timer_settings = {})
            : m_id(id),
              m_state("log_" + std::to_string(id) + ".txt"),
              m_cluster(cluster),
              m_client(std::move(client)),
              m_server(m_event_queue),
              m_timer_settings(timer_settings),
              m_logger(spdlog::stdout_color_mt("node_" + std::to_string(id))),
              m_fsm(std::move(fsm)) {
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

        auto on_lease_timeout(const boost::system::error_code &ec);

        void reset_lease_timer();

        void shut_down_lease_timer();

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

        bool is_log_more_up_to_date(unsigned int last_log_index,
                                    unsigned int last_log_term) const;

        void append_entries(unsigned int id, unsigned int lease_id);

        void initialize_next_index();

        void initialize_match_index();

        void process_pending_requests();

        void calculate_new_commit_index();

        void update_commit_index(unsigned int commit_index);

        void clear_pending_requests();

        void run_leader_loop();

        int calculate_quorum() const;

    private:
        unsigned int m_id;
        unsigned int m_commit_index = 0;
        unsigned int m_last_applied_index = 0;
        int m_leader_id = -1;

        ServerState m_server_state = ServerState::FOLLOWER;
        PersistentState m_state;

        // Only used in leader state
        // maps ids to values
        std::unordered_map<unsigned int, unsigned int> m_next_index;
        std::unordered_map<unsigned int, unsigned int> m_match_index;

        // A map of ID's and corresponding IPs
        utils::ClusterMap m_cluster;

        utils::EventQueue<Event> m_event_queue;
        bool m_running = false;

        boost::asio::io_context m_io = boost::asio::io_context();
        boost::asio::strand<boost::asio::io_context::executor_type> m_strand = make_strand(m_io);
        // Work guard is needed to keep the io context running when there is no work
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        m_work_guard = make_work_guard(m_io);

        utils::Timer m_election_timer = utils::Timer(m_io);
        utils::Timer m_heartbeat_timer = utils::Timer(m_io);
        utils::Timer m_lease_timer = utils::Timer(m_io);
        TimerSettings m_timer_settings;

        std::unique_ptr<Client> m_client = nullptr;
        gRPCServer m_server;

        std::shared_ptr<spdlog::logger> m_logger;

        // This struct stores requests that are
        // waiting for a log to be commited
        // before getting a response
        struct PendingRequest {
            // this stores the commit index value needed before a response can be sent
            unsigned int waiting_commit_index;
            std::function<void(const ClientRequestResponse &response)> callback;
        };

        std::vector<PendingRequest> m_pending_requests;

        std::shared_ptr<fsm::FSM> m_fsm;
    };

    std::string server_state_to_str(ServerState state);
} // namespace raft

#endif // NODE_H
