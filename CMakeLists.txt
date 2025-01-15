cmake_minimum_required(VERSION 3.29)
project(db)

set(CMAKE_CXX_STANDARD 20)

add_executable(db main.cpp)

target_precompile_headers(db PRIVATE libs/httplib/httplib.h)

target_include_directories(db PRIVATE libs/httplib)
