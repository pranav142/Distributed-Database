//
// Created by pknadimp on 2/18/25.
//

#ifndef CONSISTENT_HASHING_H
#define CONSISTENT_HASHING_H

#include <functional>
#include <string>
#include <utility>
#include <set>
#include <optional>

namespace utils {
    class ConsistentHashing {
    public:
        ConsistentHashing(unsigned int replicas,
                          std::function<std::size_t(const std::string &)> hasher) : m_hasher(std::move(hasher)),
            m_replicas(replicas) {
        }

        ~ConsistentHashing() = default;


        void add_node(const std::string &node_name);

        void remove_node(const std::string &node_name);

        std::optional<std::string> get_node(const std::string &key);

    private:
        std::string create_virtual_node_name(const std::string &node_name, int replica);

    private:
        unsigned int m_replicas;
        std::function<std::size_t(const std::string &)> m_hasher;

        std::unordered_map<std::size_t, std::string> m_hash_to_name;
        std::set<std::size_t> m_ring;
    };
}

#endif //CONSISTENT_HASHING_H
