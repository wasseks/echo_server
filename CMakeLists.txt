cmake_minimum_required(VERSION 3.22)
project(EchoServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Boost 1.74 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} include)

# echo_server
add_executable(echo_server src/echo_server.cpp main.cpp)
target_link_libraries(echo_server ${Boost_LIBRARIES} Threads::Threads)

# Tests with ThreadSanitizer
add_executable(test_server_tsan tests/test_server.cpp src/echo_server.cpp)
target_compile_options(test_server_tsan PRIVATE -fsanitize=thread -g)
target_link_options(test_server_tsan PRIVATE -fsanitize=thread)
target_link_libraries(test_server_tsan ${Boost_LIBRARIES} Threads::Threads GTest::GTest GTest::Main)

# Tests with AddressSanitizer
add_executable(test_server_asan tests/test_server.cpp src/echo_server.cpp)
target_compile_options(test_server_asan PRIVATE -fsanitize=address -g)
target_link_options(test_server_asan PRIVATE -fsanitize=address)
target_link_libraries(test_server_asan ${Boost_LIBRARIES} Threads::Threads GTest::GTest GTest::Main)
