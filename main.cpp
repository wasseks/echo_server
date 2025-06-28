#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::io_context& io_context, std::atomic<int>& connection_count)
        : socket_(io_context), connection_count_(connection_count) {}

    ~Session() {
        connection_count_.fetch_sub(1, std::memory_order_relaxed);
        std::cout << "Session closed, total connections: " << connection_count_ << "\n";
    }

    tcp::socket& socket() { return socket_; }

    void start() {
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    do_write(length);
                } else {
                    std::cout << "Client disconnected: " << ec.message() << "\n";
                    socket_.close(); 
                }
            });
    }

    void do_write(std::size_t length) {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(data_, length),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    do_read(); 
                } else {
                    std::cerr << "Write error: " << ec.message() << "\n";
                    socket_.close();
                }
            });
    }

    tcp::socket socket_;
    std::atomic<int>& connection_count_;
    enum { max_length = 1024 };
    char data_[max_length];
};

class Server {
public:
    Server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
          io_context_(io_context) {
        do_accept();
    }

private:
    void do_accept() {
        auto session = std::make_shared<Session>(io_context_, connection_count_);
        acceptor_.async_accept(
            session->socket(),
            [this, session](boost::system::error_code ec) {
                if (!ec) {
                    connection_count_.fetch_add(1, std::memory_order_relaxed);
                    std::cout << "New connection, total: " << connection_count_ << "\n";
                    session->start();
                } else {
                    std::cerr << "Accept error: " << ec.message() << "\n";
                }
                do_accept(); 
            });
    }

    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
    std::atomic<int> connection_count_{0};
};

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