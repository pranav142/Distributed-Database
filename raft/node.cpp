//
// Created by pknadimp on 1/18/25.

#include "node.h"
#include <functional>
#include "utils.h"

raft::Node::Node(unsigned int id, const ClusterMap &cluster, boost::asio::io_context &io) : m_id(id),
    m_cluster(cluster),
    m_state("log_" + std::to_string(id) + ".txt"),
    m_io(io),
    m_election_timer(io),
    m_strand(boost::asio::make_strand(io)) {
}

raft::ServerState raft::Node::get_server_state() const {
    return m_server_state;
}

unsigned int raft::Node::get_id() const {
    return m_id;
}

unsigned int raft::Node::get_commit_index() const {
    return m_commit_index;
}

unsigned int raft::Node::get_last_applied_index() const {
    return m_last_applied_index;
}

void raft::Node::reset_election_timer() {
    // Thread safe way to modify the election timers
    boost::asio::post(m_io, [this]() {
        m_election_timer.cancel();

        unsigned int random_time_ms = generate_random_number(ELECTION_TIMER_MIN_MS, ELECTION_TIMER_MAX_MS);
        m_election_timer.expires_after(boost::asio::chrono::milliseconds(random_time_ms));
        m_election_timer.async_wait([this](const boost::system::error_code &ec) {
            if (!ec) {
                m_event_queue.push(ElectionTimeout{});
                // reset_election_timer();
            } else if (ec != boost::asio::error::operation_aborted) {
                std::cout << "Election timer error: " << ec.message() << std::endl;
            }
        });
    });
}

void raft::Node::initialize() {
    reset_election_timer();
}

// TODO: Fix This
void raft::Node::stop() {
    m_running = false;
    m_election_timer.cancel();
}

void raft::Node::handle_election_timeout() {
    // When an election timeout occurs (Leaders do not have election timeouts):
    // 1. increment the term
    // 2. vote for self
    // 3. move to candidate state
    // 4. reset election timer
    std::cout << "Election timeout: " << server_state_to_str(m_server_state) << " term: " << m_state.get_current_term()
            << std::endl;
    if (m_server_state == ServerState::FOLLOWER || m_server_state == ServerState::CANDIDATE) {
        m_state.increment_term();
        m_state.set_voted_for(m_id);
        become_candidate();
        reset_election_timer();
    }
}

void raft::Node::election_timeout_cb() {
    m_event_queue.push(ElectionTimeout{});
    // reset_election_timer();
}

void raft::Node::become_follower(unsigned int term) {
    m_server_state = ServerState::FOLLOWER;
    m_state.set_current_term(term);
}

void raft::Node::become_leader() {
    m_server_state = ServerState::LEADER;
}

void raft::Node::run() {
    m_running = true;

    initialize();
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
         work_guard = boost::asio::make_work_guard(m_io);

    std::thread timer_thread([this] {
        m_io.run();
    });

    // TODO: Figure out how to run a seperate io thread
    while (m_running) {
        Event event = m_event_queue.pop();
        std::visit([this](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ElectionTimeout>) {
                handle_election_timeout();
            }
        }, event);
    }

    work_guard.reset();
    m_io.stop();
    timer_thread.join();
}

std::string raft::server_state_to_str(ServerState state) {
    switch (state) {
        case ServerState::FOLLOWER:
            return "Follower";
        case ServerState::CANDIDATE:
            return "Candidate";
        case ServerState::LEADER:
            return "Leader";
        default:
            return "";
    }
}

void raft::Node::become_candidate() {
    m_server_state = ServerState::CANDIDATE;
}
