#include "session.h"

#include <iostream>
#include <asio.hpp>
#include <asio/ts/buffer.hpp>


Session::Session(asio::ip::tcp::socket socket) : socket_(std::move(socket)) {}

void Session::start() {
    this->do_read();
}

// void Session::read_command() {
//     auto self(shared_from_this());
//     asio::async_read_until(this->socket_, this->data, "\r\n",
//         [this, self](std::error_code ec, std::size_t length) {
//             if (!ec) {
//                 std::istream is(&data_);
//                 std::string command;
//                 std::getline(is, command);
//                 if (!line.empty() && line.back() == '\r') {
//                     line.pop_back();
//                 }

//                 // Inform the cache we received a command, and ask it what to do next. 

                
//             }
//         });
// }

void Session::do_read() {
    // maintain a shared pointer to the session in the lambda to prevent the session from destructing before
    // the callback completes
    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(data_, max_length),
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                // Determine if we have a full command

                // Process the command

                do_write(length);
            } else {
                // log the error
            }
        });
}

void Session::do_write(size_t length) {
    auto self(shared_from_this());
    asio::async_write(socket_, asio::buffer(data_, length),
        [this, self](std::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                do_read();
            }
        });
}