cmake_minimum_required(VERSION 3.29)
project(db)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

set (RAFT_FILES raft/logs.cpp
        raft/logs.h
        Google_tests/raft_logs_tests.cpp
        raft/persistent_state.cpp
        raft/persistent_state.h
)

add_executable(db main.cpp
        DB.cpp
        wol.cpp
        wol.h
)

target_include_directories(db PRIVATE libs/httplib)

enable_testing()

add_executable(
        GTests_run
        ${RAFT_FILES}
)

target_include_directories(GTests_run PRIVATE raft)

target_link_libraries(
        GTests_run
        GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(GTests_run)
