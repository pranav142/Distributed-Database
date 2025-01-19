cmake_minimum_required(VERSION 3.29)
project(db)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

set(RAFT_FILES raft/logs.cpp
        raft/logs.h
        raft/persistent_state.cpp
        raft/persistent_state.h
        raft/error_codes.h
        raft/utils.cpp
        raft/utils.h
        raft/node.cpp
        raft/node.h
        raft/cluster.cpp
        raft/cluster.h
)

add_library(raft ${RAFT_FILES})

target_include_directories(raft PRIVATE libs/asio/include)

add_executable(db main.cpp
        DB.cpp
        wol.cpp
        wol.h
)

target_include_directories(db PRIVATE libs/httplib)

enable_testing()

add_executable(
        GTests_run
        Google_tests/raft_logs_tests.cpp
        Google_tests/raft_utils_tests.cpp
        Google_tests/raft_persistent_state_tests.cpp
)

target_link_libraries(GTests_run raft)

target_include_directories(GTests_run PRIVATE raft)

target_link_libraries(
        GTests_run
        GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(GTests_run)
