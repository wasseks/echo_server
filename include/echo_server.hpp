#pragma once 
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <memory>
#include <atomic>

using boost::asio::ip::tcp;
using boost::asio::awaitable;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::io_context& io_context, std::atomic<int>& connection_count);
    ~Session();
    tcp::socket& socket();
    awaitable<void> start();

private:
    tcp::socket socket_;
    std::atomic<int>& connection_count_;
    bool is_closed_ = false;
    enum { max_length = 1024 };
    char data_[max_length];
};

class Server {
public:
    Server(boost::asio::io_context& io_context, short port);
    ~Server();
    int get_connection_count() const;

private:
    awaitable<void> do_accept();

    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
    std::atomic<int> connection_count_{0};
};