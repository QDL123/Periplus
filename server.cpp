#include "server.h"
#include "session.h"

#include <iostream>
#include <memory>
#include <asio.hpp>
#include <asio/ts/internet.hpp>


TcpServer::TcpServer(asio::io_context& io_context, short port) 
    : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
    do_accept();
}

void TcpServer::do_accept() {
    this->acceptor_.async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                std::cout << "Starting session" << '\n';
                std::make_shared<Session>(std::move(socket))->start();
            }
            std::cout << "Session ending, looking for new connection" << std::endl;
            do_accept();
        });
}