#pragma once 
#include <boost/asio.hpp>
#include <atomic>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::io_context& io_context, std::atomic<int>& connection_count);
    ~Session();
    tcp::socket& socket();
    void start();

private:
    void do_read();
    void do_write(std::size_t length);

    tcp::socket socket_;
    std::atomic<int>& connection_count_;
    enum { max_length = 1024 };
    char data_[max_length];
};

class Server {
public:
    Server(boost::asio::io_context& io_context, short port);
    ~Server();
    int get_connection_count() const;

private:
    void do_accept();

    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
    std::atomic<int> connection_count_{0};
};