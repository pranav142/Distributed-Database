//
// Created by pknadimp on 1/17/25.
//

#include <gtest/gtest.h>
#include "persistent_state.h"
#include <fstream>

#include "utils.h"

TEST(WritePersistentStateTest, HandlesValidState) {
    raft::PersistentState state("state_test.txt");

    state.set_current_term(10);
    state.set_voted_for(20);

    auto ret = state.save_state();
    GTEST_ASSERT_EQ(ret, raft::ErrorCode::SUCCESS);
}

TEST(ReadPersistentStateTest, HandlesValidState) {
    std::ofstream file("read_test.txt");
    file << "Header,10,40\n";
    file.close();

    raft::PersistentState state("read_test.txt");

    auto ret = state.load_state();
    GTEST_ASSERT_EQ(ret, raft::ErrorCode::SUCCESS);
    GTEST_ASSERT_EQ(state.get_current_term(), 10);
    GTEST_ASSERT_EQ(state.get_voted_for(), 40);
}

// For now just mamually check if values are correct
TEST(PersistentStateAppendLog, HandlesValidLog) {
    raft::clear_file("append_log.txt");
    raft::PersistentState state("append_log.txt");
    state.set_current_term(10);

    auto ret = state.append_log("x->3");
    GTEST_ASSERT_EQ(ret, raft::ErrorCode::SUCCESS);
    ret = state.append_log("x->5");
    GTEST_ASSERT_EQ(ret, raft::ErrorCode::SUCCESS);

    state.set_current_term(20);
    ret = state.append_log("x->6");
    GTEST_ASSERT_EQ(ret, raft::ErrorCode::SUCCESS);
}

TEST(PersistentStateReadLog, HandlesValidLog) {
    raft::PersistentState state("append_log.txt");
    state.set_current_term(10);

    auto ret = state.append_log("x->3");
    GTEST_ASSERT_EQ(ret, raft::ErrorCode::SUCCESS);
    ret = state.append_log("x->5");
    GTEST_ASSERT_EQ(ret, raft::ErrorCode::SUCCESS);

    state.set_current_term(20);
    ret = state.append_log("x->6");
    GTEST_ASSERT_EQ(ret, raft::ErrorCode::SUCCESS);

    auto log = state.read_log(3);
    GTEST_ASSERT_TRUE(log != std::nullopt);
    GTEST_ASSERT_EQ(log.value().entry, "x->6");
    GTEST_ASSERT_EQ(log.value().term, 20);
    GTEST_ASSERT_EQ(log.value().index, 3);
}

TEST(PersistentStateReadLog, GetsEntries) {
    raft::PersistentState state("append_log_2.txt");
    state.set_current_term(10);

    auto ret = state.append_log("x->3");
    GTEST_ASSERT_EQ(ret, raft::ErrorCode::SUCCESS);
    ret = state.append_log("x->5");
    GTEST_ASSERT_EQ(ret, raft::ErrorCode::SUCCESS);

    state.set_current_term(20);
    ret = state.append_log("x->6");
    GTEST_ASSERT_EQ(ret, raft::ErrorCode::SUCCESS);

    auto log = state.read_log(3);
    GTEST_ASSERT_TRUE(log != std::nullopt);
    GTEST_ASSERT_EQ(log.value().entry, "x->6");
    GTEST_ASSERT_EQ(log.value().term, 20);
    GTEST_ASSERT_EQ(log.value().index, 3);

    state.add_entries(3, "3,10,x->5\n4,10,x->5\n5,10,x->5\n");
    auto logs = state.get_entries_till_end(1);
    std::cout << logs;

    log = state.read_log(3);
    GTEST_ASSERT_TRUE(log != std::nullopt);
    GTEST_ASSERT_EQ(log.value().entry, "x->5");
    GTEST_ASSERT_EQ(log.value().term, 10);
    GTEST_ASSERT_EQ(log.value().index, 3);

    state.delete_logs(1);
}

