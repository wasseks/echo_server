#include "echo_server.hpp"
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <iostream>

Session::Session(boost::asio::io_context& io_context, std::atomic<int>& connection_count)
    : socket_(io_context), connection_count_(connection_count) {}

Session::~Session() {
    if (socket_.is_open()) {
        socket_.close();
    }
    connection_count_.fetch_sub(1, std::memory_order_relaxed);
}

tcp::socket& Session::socket() { return socket_; }

awaitable<void> Session::start() {
    auto self = shared_from_this();
    try {
        while (!is_closed_ && socket_.is_open()) {
            auto [ec, length] = co_await socket_.async_read_some(
                boost::asio::buffer(data_, max_length), boost::asio::as_tuple(boost::asio::use_awaitable));
            if (ec) {
                is_closed_ = true;
                co_return;
            }
            auto [write_ec, _] = co_await boost::asio::async_write(
                socket_, boost::asio::buffer(data_, length), boost::asio::as_tuple(boost::asio::use_awaitable));
            if (write_ec) {
                is_closed_ = true;
                co_return;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in session: " << e.what() << "\n";
        is_closed_ = true;
    }
}

Server::Server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context), io_context_(io_context) {
    acceptor_.open(tcp::v4());
    acceptor_.bind(tcp::endpoint(tcp::v4(), port));
    acceptor_.listen();
    boost::asio::co_spawn(io_context, do_accept(), boost::asio::detached);
}

Server::~Server() {
    acceptor_.close();
}

int Server::get_connection_count() const {
    return connection_count_.load(std::memory_order_relaxed);
}

awaitable<void> Server::do_accept() {
    while (acceptor_.is_open()) {
        try {
            auto session = std::make_shared<Session>(io_context_, connection_count_);
            auto [ec] = co_await acceptor_.async_accept(
                session->socket(), boost::asio::as_tuple(boost::asio::use_awaitable));
            if (!ec) {
                connection_count_.fetch_add(1, std::memory_order_relaxed);
                boost::asio::co_spawn(io_context_, session->start(), boost::asio::detached);
            } else {
                std::cerr << "Accept error: " << ec.message() << "\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception in accept: " << e.what() << "\n";
        }
    }
}