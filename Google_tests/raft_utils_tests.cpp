//
// Created by pknadimp on 1/17/25.
//

#include <gtest/gtest.h>
#include <fstream>
#include "utils.h"

TEST(ReplaceLineTest, HandlesValidReplacement) {
    std::ofstream file("example.txt");
    file << "Hello World!\n";
    file << "Whats Up\n";
    file.close();

    auto ret = raft::replace_line("example.txt", 1, "Goodbye World\n");
    GTEST_ASSERT_EQ(ret, raft::ErrorCode::SUCCESS);

    std::ifstream r_file("example.txt");
    std::string line;
    std::getline(r_file, line);

    GTEST_ASSERT_EQ(line, "Goodbye World");
}
