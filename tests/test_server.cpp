#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include "echo_server.hpp"

using boost::asio::ip::tcp;

TEST(EchoServerTest, EchoFunctionality) {
    boost::asio::io_context io_context;
    Server server(io_context, 12345);
    std::thread server_thread([&io_context] { io_context.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    boost::asio::io_context client_context;
    tcp::socket client_socket(client_context);
    client_socket.connect(tcp::endpoint(boost::asio::ip::make_address_v4("127.0.0.1"), 12345));

    std::string message = "Hello, server!";
    boost::asio::write(client_socket, boost::asio::buffer(message));

    char buffer[1024];
    boost::system::error_code ec;
    std::size_t length = client_socket.read_some(boost::asio::buffer(buffer), ec);
    ASSERT_FALSE(ec);
    ASSERT_EQ(std::string(buffer, length), message);

    client_socket.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    io_context.stop();
    server_thread.join();
}

TEST(EchoServerTest, ConnectionCount) {
    boost::asio::io_context io_context;
    Server server(io_context, 12345);
    std::thread server_thread([&io_context] { io_context.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    boost::asio::io_context client_context1, client_context2;
    tcp::socket client1(client_context1), client2(client_context2);
    client1.connect(tcp::endpoint(boost::asio::ip::make_address_v4("127.0.0.1"), 12345));
    client2.connect(tcp::endpoint(boost::asio::ip::make_address_v4("127.0.0.1"), 12345));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(server.get_connection_count(), 2);

    client1.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(server.get_connection_count(), 1);

    client2.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(server.get_connection_count(), 0);

    tcp::socket client3(client_context1);
    ASSERT_NO_THROW(client3.connect(tcp::endpoint(boost::asio::ip::make_address_v4("127.0.0.1"), 12345)));

    client3.close();
    io_context.stop();
    server_thread.join();
}

TEST(EchoServerTest, StressTest) {
    boost::asio::io_context io_context;
    Server server(io_context, 12345);
    const int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&io_context] { io_context.run(); });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const int num_clients = 2000;
    std::vector<std::unique_ptr<tcp::socket>> clients;
    std::vector<boost::asio::io_context> client_contexts(num_clients);

    // Подключаем 2000 клиентов
    for (int i = 0; i < num_clients; ++i) {
        clients.emplace_back(std::make_unique<tcp::socket>(client_contexts[i]));
        ASSERT_NO_THROW(clients[i]->connect(tcp::endpoint(boost::asio::ip::make_address_v4("127.0.0.1"), 12345)));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_EQ(server.get_connection_count(), num_clients);

    std::string message = "Stress test!";
    for (int i = 0; i < num_clients; ++i) {
        boost::asio::write(*clients[i], boost::asio::buffer(message));
        char buffer[1024];
        boost::system::error_code ec;
        std::size_t length = clients[i]->read_some(boost::asio::buffer(buffer), ec);
        ASSERT_FALSE(ec);
        ASSERT_EQ(std::string(buffer, length), message);
    }

    for (auto& client : clients) {
        client->close();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ASSERT_EQ(server.get_connection_count(), 0);

    io_context.stop();
    for (auto& t : threads) {
        t.join();
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}