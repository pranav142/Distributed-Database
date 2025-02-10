//
// Created by pknadimp on 1/18/25.
//

#ifndef NODE_H
#define NODE_H

#include <vector>
#include <boost/asio.hpp>
#include "cluster.h"
#include "events.h"
#include "event_queue.h"
#include "persistent_state.h"
#include "client.h"
#include "gRPC_server.h"


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
        Node(unsigned int id, const ClusterMap &cluster, boost::asio::io_context &io, std::unique_ptr<Client> client);

        ServerState get_server_state() const;

        unsigned int get_id() const;

        unsigned int get_commit_index() const;

        unsigned int get_last_applied_index() const;

        void reset_election_timer();

        void reset_heartbeat_timer();

        void shut_down_heartbeat_timer();

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

        // Timer Stuff
        boost::asio::io_context &m_io;
        boost::asio::steady_timer m_election_timer;
        boost::asio::steady_timer m_heartbeat_timer;

        boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
        // Work guard is needed to keep the io context running even when there is no work
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_work_guard;

        std::unique_ptr<Client> m_client;
        std::unique_ptr<grpc::Server> m_server;
        std::unique_ptr<RaftSeverImpl> m_service;
    };

    std::string server_state_to_str(ServerState state);
}


#endif //NODE_H
