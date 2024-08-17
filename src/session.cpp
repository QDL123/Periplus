#include "cache.h"
#include "session.h"

#include <iostream>
#include <functional>
#include <asio.hpp>
#include <asio/ts/buffer.hpp>


Session::Session(asio::ip::tcp::socket socket, Cache *cache) : args(nullptr), socket_(std::move(socket)), cache(cache) {}

void Session::start() {
    this->read_command();
}

void Session::read_command() {
    auto self(shared_from_this());
    asio::async_read_until(this->socket_, this->input_stream, "\r\n",
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                std::istream is(&this->input_stream);
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
    auto self(shared_from_this());
    this->args = args;

    // Check what data has already been read into the buffer
    if (this->input_stream.size() >= this->args->get_static_size()) {
        // Already read in enough data to get the static args
        std::istream is(&this->input_stream);
        this->args->deserialize_static(is);
        this->read_dynamic_args(args);
        // Read dynamic data
    } else {
        // Need to read more data to deserialize static args, retreive the difference of what's needed and what's already read into the buffer
        asio::async_read(this->socket_, this->input_stream, asio::transfer_exactly(this->args->get_static_size() - this->input_stream.size()),
        [this, self, &args](std::error_code ec, std::size_t bytes_transferred) {
            if (!ec) {
                std::istream is(&this->input_stream);
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

    if (this->input_stream.size() >= this->args->size + 2) {
        std::istream is(&this->input_stream);
        this->args->deserialize_dynamic(is);
        this->cache->process_args(self);
    } else {
        // Need to read more data
        asio::async_read(this->socket_, this->input_stream, asio::transfer_exactly(this->args->size + 2 - this->input_stream.size()),
        [this, self, &args](std::error_code ec, std::size_t length) {
            if (!ec) {
                std::istream is(&this->input_stream);
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

    asio::async_read(this->socket_, this->input_stream, asio::transfer_exactly(this->args->get_static_size()),
        [this, self, &args](std::error_code ec, std::size_t bytes_transferred) {
            std::cout << "Updated buffer size: " << this->input_stream.size() << std::endl;
            if (!ec) {
                std::istream is(&this->input_stream);
                this->args->deserialize_static(is);
                // Read the dynamic data (size +2 is for the end delimiter (2 chars 1 byte each))
                asio::async_read(this->socket_, this->input_stream, asio::transfer_exactly(this->args->size + 2 - this->input_stream.size()), 
                    [this, self, &args](std::error_code ec, std::size_t length) {
                        if (!ec) {
                            std::istream dynamic_is(&this->input_stream);
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

void Session::async_write(size_t length) {
    auto self(shared_from_this());
    asio::async_write(socket_, asio::buffer(output_buf, length),
        [this, self](std::error_code ec, std::size_t /*length*/) {
            if (ec) {
                std::cout << "An error occurred responding to the client" << std::endl;
            }
        });
}

void Session::sync_write(size_t length) {
    asio::error_code ec;
    asio::write(socket_, asio::buffer(this->output_buf, length), ec);

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