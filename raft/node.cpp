//
// Created by pknadimp on 1/18/25.

#include "node.h"
#include <functional>
#include "utils.h"

raft::Node::Node(unsigned int id, const ClusterMap &cluster, boost::asio::io_context &io) : m_id(id),
    m_cluster(cluster),
    m_state("log_" + std::to_string(id) + ".txt"), m_io(io), m_election_timer(io) {
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
    unsigned int random_time_ms = generate_random_number(150, 300);
    m_election_timer.expires_after(boost::asio::chrono::milliseconds(random_time_ms));
    m_election_timer.async_wait(std::bind(&Node::handle_election_timeout, this));
}

void raft::Node::start() {
    reset_election_timer();
}

void raft::Node::handle_election_timeout() {
    std::cout << "Election timeout: " << server_state_to_str(m_server_state) << " term: " << m_state.get_current_term() << std::endl;
    if (m_server_state == ServerState::FOLLOWER || m_server_state == ServerState::CANDIDATE) {
        m_server_state = ServerState::CANDIDATE;
        m_state.increment_term();
        reset_election_timer();
    }
}

void raft::Node::become_follower(unsigned int term) {
    m_server_state = ServerState::FOLLOWER;
    m_state.set_current_term(term);
}

void raft::Node::become_leader() {
    m_server_state = ServerState::LEADER;
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
