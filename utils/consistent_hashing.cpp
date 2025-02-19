//
// Created by pknadimp on 2/18/25.
//

#include "consistent_hashing.h"

std::string utils::ConsistentHashing::create_virtual_node_name(const std::string &node_name, int replica) {
    return node_name + "_" + std::to_string(replica);
}

void utils::ConsistentHashing::add_node(const std::string &node_name) {
    for (int i = 0; i < m_replicas; i++) {
        std::size_t hash = m_hasher(create_virtual_node_name(node_name, i));
        m_hash_to_name[hash] = node_name;
        m_ring.insert(hash);
    }
}

void utils::ConsistentHashing::remove_node(const std::string &node_name) {
    for (int i = 0; i < m_replicas; i++) {
        std::size_t hash = m_hasher(create_virtual_node_name(node_name, i));
        m_hash_to_name.erase(hash);
        m_ring.erase(hash);
    }
}

std::optional<std::string> utils::ConsistentHashing::get_node(const std::string &key) {
    if (m_ring.empty() || m_hash_to_name.empty()) {
        return std::nullopt;
    }

    std::size_t hash = m_hasher(key);

    // the first value that is equal
    // or is closest yet still ahead of the hash
    auto it = m_ring.lower_bound(hash);

    if (it == m_ring.end()) {
        // wrap around to beginning of the ring
        it = m_ring.begin();
    }

    return m_hash_to_name[*it];
}
