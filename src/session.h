/*
The session object is responsible for abstracting away everything to do with the network away from the 
caching logic. It knows how to read and write data, and holds a reference to the cache which it uses
to process incoming data and then figure out what data to send back.
*/

#ifndef SESSION_H
#define SESSION_H

#include "args.h"
#include "cache.h"

#include <vector>
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>


class Session : public std::enable_shared_from_this<Session> {
public:

    // TODO: Add cache weak ptr here (Sessions should not impact the cache lifecycle which is owned by the server)
    // This will alleviate the need to pass callback functions everywhere.
    explicit Session(asio::ip::tcp::socket socket, Cache *cache);

    void start();
    void read_command();
    void read_static_args(std::shared_ptr<Args> args);
    void updated_read_args(std::shared_ptr<Args> args);
    void read_dynamic_args(std::shared_ptr<Args> args);
    void do_write(std::size_t length);
    void sync_write(std::size_t length);

    enum { max_length = 1024 };
    char output_buf[max_length];
    std::shared_ptr<Args> args;
    Cache *cache;

    ~Session();

private:
    asio::ip::tcp::socket socket_;
    asio::streambuf input_stream;
    void do_read();
};

#endif