//
// Created by pknadimp on 1/18/25.

#include "node.h"

#include <utility>

raft::Node::Node(unsigned int id, const ClusterMap& cluster) : m_id(id), m_cluster(cluster),
                                                        m_state("log_" + std::to_string(id) + ".txt") {
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




