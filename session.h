#ifndef SESSION_H
#define SESSION_H

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>


class Session : public std::enable_shared_from_this<Session> {
public:

    Session(asio::ip::tcp::socket socket);

    void start();

private:
    asio::ip::tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
    // asio::streambuf data_;

    // void read_command();
    void do_read();

    void do_write(std::size_t length);
};

#endif