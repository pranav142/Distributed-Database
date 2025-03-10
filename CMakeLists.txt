cmake_minimum_required(VERSION 3.29)
SET(CMAKE_CXX_COMPILER clang)
project(db LANGUAGES CXX)

find_package(TBB REQUIRED)
find_package(Crow REQUIRED)
find_package(CURL REQUIRED)

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
        utils/serialized_data.cpp
        utils/serialized_data.h
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
        ${CMAKE_CURRENT_SOURCE_DIR}/FSM
)

target_link_libraries(raft PUBLIC protolib grpc++ spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32> utils fsm)

set(LB_FILES
        load_balancer/lb.cpp
        load_balancer/lb.h
        load_balancer/gRPC_client.cpp
        load_balancer/gRPC_client.h
        load_balancer/http_server.cpp
        load_balancer/http_server.h
        load_balancer/event.h
)

add_library(lb
        ${LB_FILES}
)

target_include_directories(lb
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/spdlog/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/Crow/include
)

target_link_libraries(lb PUBLIC protolib grpc++ spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32> utils TBB::tbb Crow::Crow)

set(FSM_FILES
        FSM/key_value/db.cpp
        FSM/key_value/db.h
        FSM/key_value/command.cpp
        FSM/key_value/command.h
        FSM/FSM.h
)

add_library(fsm
        ${FSM_FILES}
)

target_include_directories(fsm PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/FSM
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/json/single_include
)

target_link_libraries(fsm PUBLIC utils)

add_executable(main
        main.cpp
)

target_link_libraries(main
        PRIVATE
        raft
        lb
        fsm
)

target_include_directories(main
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/asio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/httplib
        ${CMAKE_CURRENT_SOURCE_DIR}/raft
)

enable_testing()

add_executable(GTest_FSM_run
        Google_tests/FSM_tests/kv_command_tests.cpp
        Google_tests/FSM_tests/kv_db_tests.cpp
)

target_link_libraries(GTest_FSM_run
        PRIVATE
        fsm
        GTest::gtest_main
)

target_include_directories(GTest_FSM_run
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/FSM
)

add_executable(GTest_raft_run
        Google_tests/raft_tests/raft_logs_tests.cpp
        Google_tests/raft_tests/raft_node_tests.cpp
        Google_tests/raft_tests/raft_persistent_state_tests.cpp
        Google_tests/raft_tests/raft_utils_tests.cpp
)

target_link_libraries(GTest_raft_run
        PRIVATE
        raft
        lb
        GTest::gtest_main
)

target_include_directories(GTest_raft_run
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/raft
        ${CMAKE_CURRENT_SOURCE_DIR}/load_balancer
)

add_executable(GTest_util_run
        ${CMAKE_CURRENT_SOURCE_DIR}/Google_tests/util_tests/util_consistent_hashing_tests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Google_tests/util_tests/util_event_queue_tests.cpp
)

target_link_libraries(GTest_util_run
        PRIVATE
        utils
        GTest::gtest_main
)

target_include_directories(GTest_util_run
        PRIVATE
        utils
)

add_executable(GTest_lb_run
        ${CMAKE_CURRENT_SOURCE_DIR}/Google_tests/loadbalancer_tests/lb_tests.cpp
        Google_tests/loadbalancer_tests/http_server_tests.cpp
)

target_link_libraries(GTest_lb_run
        PRIVATE
        lb
        fsm
        raft
        CURL::libcurl
        GTest::gtest_main
)

target_include_directories(GTest_lb_run
        PRIVATE
        load_balancer
        FSM
        raft
        ${CMAKE_CURRENT_SOURCE_DIR}
)

include(GoogleTest)
gtest_discover_tests(GTest_raft_run GTest_FSM_run GTest_util_run)
