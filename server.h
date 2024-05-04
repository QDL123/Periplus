#ifndef SERVER_H
#define SERVER_H

#include "cache.h"
#include <asio.hpp>
#include <asio/ts/internet.hpp>

class TcpServer {
public:
    TcpServer(asio::io_context& io_context, short port);

private:
    asio::ip::tcp::acceptor acceptor_;
    std::unique_ptr<Cache> cache;


    void do_accept();
};



#endif