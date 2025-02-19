cmake_minimum_required(VERSION 3.29)
project(db LANGUAGES CXX)

find_package(TBB REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# --- Fetch GoogleTest ---
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip  # Example version
)

FetchContent_MakeAvailable(googletest)

find_package(spdlog REQUIRED)

# Installs GRPC
set(ABSL_ENABLE_INSTALL ON)
set(GRPC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/libs/grpc)
add_subdirectory(${GRPC_ROOT})

find_package(Protobuf CONFIG REQUIRED)

set(PROTO_FILES proto/raft.proto)
add_library(protolib ${PROTO_FILES})
target_link_libraries(protolib PRIVATE grpc++)
target_include_directories(protolib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate(LANGUAGE cpp TARGET protolib)
protobuf_generate(LANGUAGE grpc TARGET protolib
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        PLUGIN "protoc-gen-grpc=${CMAKE_CURRENT_BINARY_DIR}/libs/grpc/grpc_cpp_plugin"
)

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
        raft/events.h
        raft/client.h
        raft/gRPC_client.cpp
        raft/gRPC_client.h
        raft/gRPC_server.cpp
        raft/gRPC_server.h
        raft/client.cpp
        raft/FSM.h
)

set(UTIL_FILES
        utils/logging.cpp
        utils/logging.h
        utils/timer.h
        utils/timer.cpp
        utils/event_queue.h
        utils/cluster.h
        utils/consistent_hashing.cpp
        utils/consistent_hashing.h
)

add_library(utils
        ${UTIL_FILES}
)

target_include_directories(utils
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/asio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/spdlog/include
)

target_link_libraries(utils PUBLIC spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

add_library(raft
        ${RAFT_FILES}
)

target_include_directories(raft
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/asio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/spdlog/include
        ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

target_link_libraries(raft PUBLIC protolib grpc++ spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32> utils)

set(LB_FILES
        load_balancer/lb.cpp
        load_balancer/lb.h
)

add_library(lb
        ${LB_FILES}
)

target_include_directories(lb
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/spdlog/include
)

target_link_libraries(lb PUBLIC protolib grpc++ spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32> utils TBB::tbb)

set(DB_FILES
        db/db.cpp
        db/db.h
        db/command.cpp
        db/command.h
)

add_library(db
        ${DB_FILES}
)

add_executable(main
        main.cpp
)

target_link_libraries(main
        PRIVATE
        raft
)

target_include_directories(main
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/asio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/httplib
)

enable_testing()

add_executable(GTests_run
        Google_tests/raft_logs_tests.cpp
        Google_tests/raft_utils_tests.cpp
        Google_tests/raft_persistent_state_tests.cpp
        Google_tests/raft_event_queue_tests.cpp
        Google_tests/raft_node_tests.cpp
        Google_tests/db_command_tests.cpp
        Google_tests/db_tests.cpp
        Google_tests/utils_consistent_hashing_tests.cpp
)

target_link_libraries(GTests_run
        PRIVATE
        raft
        lb
        db
        utils
        GTest::gtest_main
)

target_include_directories(GTests_run
        PRIVATE
        # ${CMAKE_CURRENT_SOURCE_DIR}/libs/asio/include
        # ${CMAKE_CURRENT_SOURCE_DIR}/libs/httplib
        raft
        load_balancer
        utils
        db
        TBB::tbb
)

include(GoogleTest)
gtest_discover_tests(GTests_run)
