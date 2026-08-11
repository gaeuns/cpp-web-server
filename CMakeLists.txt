cmake_minimum_required(VERSION 3.10)
project(cpp_multithread_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(server
    src/main.cpp
    src/ThreadPool.cpp
    src/Server.cpp
)

target_include_directories(server PRIVATE include)
target_link_libraries(server PRIVATE Threads::Threads)
