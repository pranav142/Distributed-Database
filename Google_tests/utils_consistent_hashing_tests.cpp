//
// Created by pknadimp on 2/18/25.
//

#include <gtest/gtest.h>

#include "consistent_hashing.h"

std::size_t test_hasher(const std::string &s) {
    std::size_t hash = 0;
    for (char c : s) {
        hash = hash * 31 + static_cast<unsigned int>(c);
    }
    return hash;
}

TEST(ConsistentHashingTests, NoNodesReturnsNullopt) {
    utils::ConsistentHashing ring(3, test_hasher);
    auto node = ring.get_node("somekey");
    EXPECT_FALSE(node.has_value());  // Expect no node when none have been added.
}

TEST(ConsistentHashingTests, SingleNodeMapping) {
    utils::ConsistentHashing ring(3, test_hasher);
    ring.add_node("NodeA");

    auto node = ring.get_node("apple");
    ASSERT_TRUE(node.has_value());
    EXPECT_EQ(node.value(), "NodeA");
}

TEST(ConsistentHashingTests, MultipleNodesMapping) {
    utils::ConsistentHashing ring(3, test_hasher);
    ring.add_node("NodeA");
    ring.add_node("NodeB");
    ring.add_node("NodeC");

    // With multiple nodes, different keys should map to one of these nodes.
    std::vector<std::string> keys = {"apple", "banana", "cherry", "date", "fig"};
    for (const auto &key : keys) {
        auto node = ring.get_node(key);
        ASSERT_TRUE(node.has_value());
        // The node value should be one of the nodes we added.
        EXPECT_TRUE(node.value() == "NodeA" || node.value() == "NodeB" || node.value() == "NodeC")
            << "Key " << key << " mapped to unknown node " << node.value();
    }
}

TEST(ConsistentHashingTests, DeterministicMapping) {
    utils::ConsistentHashing ring(3, test_hasher);
    ring.add_node("NodeA");
    ring.add_node("NodeB");

    // The same key should always map to the same node if the ring doesn't change.
    auto node1 = ring.get_node("banana");
    auto node2 = ring.get_node("banana");
    ASSERT_TRUE(node1.has_value());
    ASSERT_TRUE(node2.has_value());
    EXPECT_EQ(node1.value(), node2.value());
}

TEST(ConsistentHashingTests, NodeRemoval) {
    utils::ConsistentHashing ring(3, test_hasher);
    ring.add_node("NodeA");
    ring.add_node("NodeB");
    ring.add_node("NodeC");

    // Get mapping before removal.
    auto beforeRemoval = ring.get_node("cherry");
    ASSERT_TRUE(beforeRemoval.has_value());

    // Remove the node that "cherry" maps to.
    std::string removedNode = std::move(beforeRemoval.value());
    ring.remove_node(removedNode);

    // Now, "cherry" should map to a different node.
    auto afterRemoval = ring.get_node("cherry");
    // It might be that "cherry" maps to a different node,
    // but if the removed node was the only one with that hash range, it should change.
    ASSERT_TRUE(afterRemoval.has_value());
    EXPECT_NE(afterRemoval.value(), removedNode);
}