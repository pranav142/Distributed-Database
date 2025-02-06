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
        std::string address;

        friend std::ostream &operator<<(std::ostream &os, const NodeInfo &node) {
            os << node.address;
            return os;
        }
    };

    typedef std::unordered_map<unsigned int, NodeInfo> ClusterMap;
}


#endif //CLUSTER_H
