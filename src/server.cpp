/*
The server manages the cache lifecycle, accepts connections, and creates session objects upon accepting those connections.
*/

#include "server.h"
#include "session.h"
#include "cache.h"

#include <iostream>
#include <memory>
#include <asio.hpp>
#include <asio/ts/internet.hpp>


TcpServer::TcpServer(asio::io_context& io_context, short port) 
    : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
    this->cache = std::make_unique<Cache>();
    do_accept();
}

void TcpServer::do_accept() {
    this->acceptor_.async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                auto session = std::make_shared<Session>(std::move(socket), this->cache.get());
                this->sessions.push_back(session);
                session->start();
            }

            std::cout << "Listening for new sessions" << std::endl;
            // Listen for more connections
            do_accept();
        });
}