#include "echo_server.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

int main() {
    try {
        boost::asio::io_context io_context;
        Server server(io_context, 12345);
        const int num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&io_context] { io_context.run(); });
        }

        std::cout << "Server running with " << num_threads << " threads\n";

        for (auto& t : threads) {
            t.join();
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}