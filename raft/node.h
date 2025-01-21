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

namespace raft {
    constexpr int ELECTION_TIMER_MIN_MS = 150;
    constexpr int ELECTION_TIMER_MAX_MS = 300;

    enum class ServerState : int {
        FOLLOWER = 0,
        CANDIDATE,
        LEADER,
    };

    class Node {
    public:
        Node(unsigned int id, const ClusterMap &cluster, boost::asio::io_context &io);

        ServerState get_server_state() const;

        unsigned int get_id() const;

        unsigned int get_commit_index() const;

        unsigned int get_last_applied_index() const;

        void reset_election_timer();

        void handle_election_timeout();

        void run();

        void stop();

    private:

        void initialize();

        void election_timeout_cb();

        void become_follower(unsigned int term);

        void become_candidate();

        void become_leader();

    private:
        unsigned int m_id;
        unsigned int m_commit_index = 0;
        unsigned int m_last_applied_index = 0;

        ServerState m_server_state = ServerState::FOLLOWER;
        PersistentState m_state;

        // Only use din leader configuration
        std::vector<unsigned int> m_next_index;
        std::vector<unsigned int> m_match_index;

        // A map of ID's and corresponding IPs
        ClusterMap m_cluster;

        // Timer Stuff
        boost::asio::io_context &m_io;
        boost::asio::steady_timer m_election_timer;

        EventQueue<Event> m_event_queue;
        bool m_running = false;
        boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    };

    std::string server_state_to_str(ServerState state);
}


#endif //NODE_H
