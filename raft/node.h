//
// Created by pknadimp on 1/18/25.
//

#ifndef NODE_H
#define NODE_H

#include "persistent_state.h"
#include <unordered_map>
#include "cluster.h"
#include <vector>

namespace raft {
    enum class ServerState : int {
        FOLLOWER = 0,
        CANDIDATE,
        LEADER,
    };

    class Node {
    public:
        Node(unsigned int id, const ClusterMap &cluster);

        ServerState get_server_state() const;

        unsigned int get_id() const;

        unsigned int get_commit_index() const;

        unsigned int get_last_applied_index() const;

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
    };
}


#endif //NODE_H
