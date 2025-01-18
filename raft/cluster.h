//
// Created by pknadimp on 1/18/25.
//

#ifndef CLUSTER_H
#define CLUSTER_H

#include <iostream>
#include <string>
#include <unordered_map>

namespace raft {
    struct NodeInfo {
        std::string ip;
        unsigned short port;

        friend std::ostream &operator<<(std::ostream &os, const NodeInfo &node) {
            os << node.ip << ":" << node.port;
            return os;
        }
    };

    typedef std::unordered_map<unsigned int, NodeInfo> ClusterMap;
}


#endif //CLUSTER_H
