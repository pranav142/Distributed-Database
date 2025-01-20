cmake_minimum_required(VERSION 3.29)
project(db LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

# --- Fetch GoogleTest ---
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip  # Example version
)
FetchContent_MakeAvailable(googletest)

set(RAFT_FILES
        raft/logs.cpp
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
        raft/event_queue.h
)

add_library(raft
        ${RAFT_FILES}
)

target_include_directories(raft
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/asio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/httplib
)

add_executable(db
        main.cpp
)

target_link_libraries(db
        PRIVATE
        raft
)

target_include_directories(db
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/asio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/httplib
)

enable_testing()

add_executable(GTests_run
        Google_tests/raft_logs_tests.cpp
        Google_tests/raft_utils_tests.cpp
        Google_tests/raft_persistent_state_tests.cpp
        Google_tests/raft_node_tests.cpp
        Google_tests/raft_event_queue_tests.cpp
)

target_link_libraries(GTests_run
        PRIVATE
        raft
        GTest::gtest_main
)

target_include_directories(GTests_run
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/asio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/httplib
        raft
)

include(GoogleTest)
gtest_discover_tests(GTests_run)
