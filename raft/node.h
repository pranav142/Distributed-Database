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

namespace raft {
    constexpr int ELECTION_TIMER_MIN_MS = 1500;
    constexpr int ELECTION_TIMER_MAX_MS = 3000;

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

        void run();

        void stop();

    private:
        void initialize();

        void shut_down_election_timer();

        void become_follower(unsigned int term);

        void become_candidate();

        void become_leader();

        void run_follower_loop();

        void run_candidate_loop();

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

        boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
        // Work guard is needed to keep the io context running even when there is no work
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_work_guard;

        std::unique_ptr<Client> m_client;
    };

    std::string server_state_to_str(ServerState state);
}


#endif //NODE_H
