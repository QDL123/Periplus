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


void Session::updated_read_args(std::shared_ptr<Args> args) {
    this->args = args;
    auto self(shared_from_this());

    // Check what data has already been read into the buffer
    if (this->data_stream_.size() >= this->args->get_static_size()) {
        // Already read in enough data to get the static args
        std::istream is(&this->data_stream_);
        this->args->deserialize_static(is);
        this->read_dynamic_args(args);
        // Read dynamic data
    } else {
        // Need to read more data to deserialize static args, retreive the difference of what's needed and what's already read into the buffer
        asio::async_read(this->socket_, this->data_stream_, asio::transfer_exactly(this->args->get_static_size() - this->data_stream_.size()),
        [this, self, &args](std::error_code ec, std::size_t bytes_transferred) {
            if (!ec) {
                std::istream is(&this->data_stream_);
                this->args->deserialize_static(is);
                // Now we can go forward with reading dynamic data
                this->read_dynamic_args(args);
            } else {
                std::cout << "AN ERROR HAS OCCURRED WHILE READING STATIC DATA" << std::endl;
            }
        });
    }
}

void Session::read_dynamic_args(std::shared_ptr<Args> args) {
    auto self(shared_from_this());

    if (this->data_stream_.size() >= this->args->size + 2) {
        std::istream is(&this->data_stream_);
        this->args->deserialize_dynamic(is);
        this->cache->process_args(self);
    } else {
        // Need to read more data
        asio::async_read(this->socket_, this->data_stream_, asio::transfer_exactly(this->args->size + 2 - this->data_stream_.size()),
        [this, self, &args](std::error_code ec, std::size_t length) {
            if (!ec) {
                std::istream is(&this->data_stream_);
                this->args->deserialize_dynamic(is);
                this->cache->process_args(self);
            } else {
                std::cout << "AN ERROR OCCURRED WHILE READYING DYNAMIC DATA" << std::endl;
            }
        });
    }
}


void Session::read_static_args(std::shared_ptr<Args> args) {
    this->args = args;
    auto self(shared_from_this());

    asio::async_read(this->socket_, this->data_stream_, asio::transfer_exactly(this->args->get_static_size()),
        [this, self, &args](std::error_code ec, std::size_t bytes_transferred) {
            std::cout << "Updated buffer size: " << this->data_stream_.size() << std::endl;
            if (!ec) {
                std::istream is(&this->data_stream_);
                this->args->deserialize_static(is);
                // Read the dynamic data (size +2 is for the end delimiter (2 chars 1 byte each))
                asio::async_read(this->socket_, this->data_stream_, asio::transfer_exactly(this->args->size + 2 - this->data_stream_.size()), 
                    [this, self, &args](std::error_code ec, std::size_t length) {
                        if (!ec) {
                            std::istream dynamic_is(&this->data_stream_);
                            this->args->deserialize_dynamic(dynamic_is);
                            this->cache->process_args(self);
                        } else {
                            std::cout << "AN ERROR HAS OCCURRED DURING DYNAMIC DESERIALIZATION" << std::endl;
                            std::cerr << ec << std::endl;
                        }   
                });
            } else {
                std::cout << "AN ERROR HAS OCCURRED DURING STATIC DESERIALIZATION" << std::endl;
                std::cerr << ec << std::endl;
            }
    });
    std::cout << "Called asio::async_read" << std::endl;
}


void Session::read_args(std::shared_ptr<Args> args) {
    this->args = args;
    auto self(shared_from_this());
    asio::async_read_until(this->socket_, this->data_stream_, "\r\n",
        [this, self, &args](std::error_code ec, std::size_t length) {
            if (!ec) {
                std::istream is(&this->data_stream_);
                this->args->deserialize(is);
                this->cache->process_args(self);
            } else {
                std::cout << "AN ERROR HAS OCCURRED DURING STATIC DESERIALIZATION" << std::endl;
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

void Session::sync_write(size_t length) {
    asio::error_code ec;
    asio::write(socket_, asio::buffer(this->data_, length), ec);

    if (!ec) {
        // std::cout << "Sent data!" << std::endl;
    } else {
        std::cout << "An error occurred while sending data to the client" << std::endl;
        std::cout << ec << std::endl;
    }
}

Session::~Session() {
    std::cout << "Session destructing" << std::endl;
}