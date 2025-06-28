cmake_minimum_required(VERSION 3.10)
project(EchoServer)

set(CMAKE_CXX_STANDARD 17)
find_package(Boost 1.65 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} include)

# echo_server
add_executable(echo_server src/echo_server.cpp main.cpp)
target_link_libraries(echo_server ${Boost_LIBRARIES} Threads::Threads)

# test echo_server
add_executable(test_server tests/test_server.cpp src/echo_server.cpp)
target_link_libraries(test_server ${Boost_LIBRARIES} Threads::Threads GTest::GTest GTest::Main)