//
// Created by pknadimp on 1/18/25.

#include "node.h"

#include <functional>
#include <utility>

raft::Node::Node(unsigned int id, const ClusterMap &cluster, boost::asio::io_context &io) : m_id(id),
    m_cluster(cluster),
    m_state("log_" + std::to_string(id) + ".txt"), m_io(io), m_election_timer(io) {
    m_election_timer.async_wait(std::bind(&Node::handle_election_timeout, this));
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
    // unsigned int random_time_ms =
}

void raft::Node::handle_election_timeout() {

}

void raft::Node::become_follower(unsigned int term) {
    m_server_state = ServerState::FOLLOWER;
    m_state.set_current_term(term);
}

void raft::Node::become_leader() {
    m_server_state = ServerState::LEADER;
}

void raft::Node::become_candidate() {
    m_server_state = ServerState::CANDIDATE;
}
