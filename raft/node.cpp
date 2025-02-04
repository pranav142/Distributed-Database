//
// Created by pknadimp on 1/18/25.

#include "node.h"
#include <functional>
#include "utils.h"
#include "../libs/grpc/src/core/lib/iomgr/timer_manager.h"

raft::Node::Node(unsigned int id, const ClusterMap &cluster, boost::asio::io_context &io,
                 std::unique_ptr<Client> client) : m_id(id),
                                                   m_cluster(cluster),
                                                   m_state("log_" + std::to_string(id) + ".txt"),
                                                   m_io(io),
                                                   m_election_timer(io),
                                                   m_work_guard(make_work_guard(io)),
                                                   m_client(std::move(client)) {
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
    m_election_timer.cancel();
    unsigned int random_time_ms = generate_random_number(ELECTION_TIMER_MIN_MS, ELECTION_TIMER_MAX_MS);
    m_election_timer.expires_after(boost::asio::chrono::milliseconds(random_time_ms));
    // TODO: Clean this to be more readable
    m_election_timer.async_wait([this](const boost::system::error_code &ec) {
        if (ec) {
            std::cout << "Election timer error: " << ec.message() << std::endl;
            return;
        }
        if (m_server_state == ServerState::FOLLOWER || m_server_state == ServerState::CANDIDATE) {
            become_candidate();
        } else {
            std::cout << "Election timeout occured while leader" << std::endl;
        }
    });
}

void raft::Node::initialize() {
    become_follower(1);
}

void raft::Node::shut_down_election_timer() {
    m_election_timer.cancel();
}

void raft::Node::become_follower(unsigned int term) {
    m_server_state = ServerState::FOLLOWER;
    // Vote for no one
    m_state.set_voted_for(-1);
    m_state.set_current_term(term);
    reset_election_timer();
}

void raft::Node::become_candidate() {
    m_server_state = ServerState::CANDIDATE;
    // Vote for self
    m_state.set_voted_for(m_id);
    m_state.increment_term();
    reset_election_timer();

    // start the election
    // Finding out how many are needed to win
    // Send off request votes to all of the nodes
    // Need to make sure that if a response comes back
}

void raft::Node::become_leader() {
    m_server_state = ServerState::LEADER;
}

// Blocking Call that causes the node to run
void raft::Node::run() {
    initialize();
    m_io.run();
}

void raft::Node::stop() {
    shut_down_election_timer();
    m_work_guard.reset();
    m_io.stop();
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
