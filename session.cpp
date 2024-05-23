#include "cache.h"
#include "session.h"

#include <iostream>
#include <functional>
#include <asio.hpp>
#include <asio/ts/buffer.hpp>


Session::Session(asio::ip::tcp::socket socket, Cache *cache) : socket_(std::move(socket)), cache(cache), args(nullptr) {}

void Session::start() {
    this->read_command();
}

void Session::read_command() {
    // do we still need this pointer if the cache is long lived and holds a reference to the session?
    // I believe the answer is yes in this async model because the caller will return right away unless we use coroutines
    auto self(shared_from_this());
    asio::async_read_until(this->socket_, this->data_stream_, "\r\n",
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                std::istream is(&this->data_stream_);
                std::string command;
                std::getline(is, command);
                if (!command.empty() && command.back() == '\r') {
                    command.pop_back();
                }

                // Inform the cache we received a command, and ask it what to do next. 
                this->cache->processCommand(self, command);
            }
        });
}


void Session::read_args(std::shared_ptr<Args> args) {
    std::cout << "Waiting for command args" << std::endl;
    this->args = args;
    auto self(shared_from_this());
    asio::async_read_until(this->socket_, this->data_stream_, "\r\n",
        [this, self, &args](std::error_code ec, std::size_t length) {
            std::cout << "Reading command args" << std::endl;
            if (!ec) {
                std::istream is(&this->data_stream_);
                std::cout << "Deserializing" << std::endl;
                this->args->deserialize(is);
                std::cout << "Processing args" << std::endl;
                this->cache->process_args(self);
            } else {
                std::cerr << ec << std::endl;
            }
    });
}

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
                // do_read();
                std::cout << "RESPONDED!" << std::endl;
            } else {
                std::cout << "An error occurred responding to the client" << std::endl;
            }
        });
}

Session::~Session() {
    std::cout << "Session destructing" << std::endl;
}