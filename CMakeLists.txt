cmake_minimum_required(VERSION 3.29)
project(db LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

protobuf_generate(TARGET raft LANGUAGE cpp)

# --- Fetch GoogleTest ---
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip  # Example version
)
FetchContent_MakeAvailable(googletest)

# --- Fetch gRPC ---
FetchContent_Declare(
        gRPC
        GIT_REPOSITORY https://github.com/grpc/grpc
        GIT_TAG        v1.70.0
)
set(ABSL_ENABLE_INSTALL ON)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(gRPC)

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
        raft/events.h
        raft/client.h
)

add_library(raft
        ${RAFT_FILES}
)

set(PROTO_FILES proto/raft.proto)
set(GENERATED_PROTO_DIR ${CMAKE_BINARY_DIR}/generated)

file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})

protobuf_generate(
        LANGUAGE cpp
        TARGET raft
        PROTOC_OUT_DIR ${GENERATED_PROTO_DIR}
        PROTOS ${PROTO_FILES}
)

target_include_directories(raft
        PUBLIC
        ${GENERATED_PROTO_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/asio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/httplib
)

target_link_libraries(raft PUBLIC grpc++)

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
