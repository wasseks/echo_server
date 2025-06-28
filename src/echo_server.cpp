#include "echo_server.hpp"
#include <iostream>

Session::Session(boost::asio::io_context& io_context, std::atomic<int>& connection_count)
    : socket_(io_context), connection_count_(connection_count) {}

Session::~Session() {
    socket_.close();
    connection_count_.fetch_sub(1, std::memory_order_relaxed);
}

tcp::socket& Session::socket() { return socket_; }

void Session::start() {
    do_read();
}

void Session::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(
        boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                do_write(length);
            } else {
                socket_.close();
            }
        });
}

void Session::do_write(std::size_t length) {
    auto self(shared_from_this());
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(data_, length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                do_read();
            } else {
                socket_.close();
            }
        });
}

Server::Server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), 
      io_context_(io_context) {
    do_accept();
}

Server::~Server() {
    acceptor_.close();
}

int Server::get_connection_count() const {
    return connection_count_.load(std::memory_order_relaxed);
}

void Server::do_accept() {
    auto session = std::make_shared<Session>(io_context_, connection_count_);
    acceptor_.async_accept(
        session->socket(),
        [this, session](boost::system::error_code ec) {
            if (!ec) {
                connection_count_.fetch_add(1, std::memory_order_relaxed);
                session->start();
            } else {
                std::cerr << "Accept error: " << ec.message() << "\n";
            }
            if (!io_context_.stopped()) {
                do_accept();
            }
        });
}