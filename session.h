#ifndef SESSION_H
#define SESSION_H

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>


class Session : public std::enable_shared_from_this<Session> {
public:

    explicit Session(asio::ip::tcp::socket socket);

    void start();
    void read_command(std::function<void(std::shared_ptr<Session> session, std::string)> process_command);

    void do_write(std::size_t length);

    enum { max_length = 1024 };
    char data_[max_length];

    ~Session();
private:
    asio::ip::tcp::socket socket_;
    asio::streambuf data_stream_;
    void do_read();
};

#endif