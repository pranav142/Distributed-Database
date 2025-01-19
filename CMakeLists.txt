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

# --- Source files for your Raft library ---
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
)

# --- Define the 'raft' library ---
add_library(raft
        ${RAFT_FILES}
)

# If your code in raft/*.h includes Boost.Asio headers:
# Provide the path to Boost.Asio (or standalone Asio) if needed:
target_include_directories(raft
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/asio/include  # Adjust if needed
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/httplib       # If raft code includes httplib
        # You can also add ${CMAKE_CURRENT_SOURCE_DIR}/raft
        # if you need direct includes like #include "node.h"
)

# If your raft library depends on any other libraries (pthread, etc.), link them here:
# target_link_libraries(raft PUBLIC Threads::Threads)

# --- The main db executable ---
add_executable(db
        main.cpp
)

# Link db with the raft library
target_link_libraries(db
        PRIVATE
        raft
)

# If 'db' itself directly includes httplib or Asio headers:
target_include_directories(db
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/asio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/httplib
)

# --- Enable testing and define your test executable ---
enable_testing()

add_executable(GTests_run
        Google_tests/raft_logs_tests.cpp
        Google_tests/raft_utils_tests.cpp
        Google_tests/raft_persistent_state_tests.cpp
        Google_tests/raft_node_tests.cpp
)

# Link test executable with raft library + GoogleTest
target_link_libraries(GTests_run
        PRIVATE
        raft
        GTest::gtest_main
)

# If your test code itself includes Asio/httplib, add includes:
target_include_directories(GTests_run
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/asio/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/httplib
        raft
)

# Discover and run tests
include(GoogleTest)
gtest_discover_tests(GTests_run)
