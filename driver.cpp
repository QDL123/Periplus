#include "server.h"

#include <iostream>
#include <memory>
#include <asio.hpp>

// #include <iostream>
// #include <memory>
// #include <asio.hpp>
// #include <asio/ts/buffer.hpp>
// #include <asio/ts/internet.hpp>


// class Session : public std::enable_shared_from_this<Session> {
// public:
//     Session(asio::ip::tcp::socket socket) : socket_(std::move(socket)) {}

//     void start() {
//         do_read();
//     }

// private:
//     asio::ip::tcp::socket socket_;
//     enum { max_length = 1024 };
//     char data_[max_length];
//     // asio::streambuf data_;

//     // void read_command() {
//     //     auto self(shared_from_this());
//     //     asio::async_read_until(this->socket_, this->data, "\r\n",
//     //         [this, self](std::error_code ec, std::size_t length) {
//     //             if (!ec) {
//     //                 std::istream is(&data_);
//     //                 std::string command;
//     //                 std::getline(is, command);
//     //                 if (!line.empty() && line.back() == '\r') {
//     //                     line.pop_back();
//     //                 }

//     //                 // Inform the cache we received a command, and ask it what to do next. 

                    
//     //             }
//     //         });
//     // }
//     void do_read() {
//         // maintain a shared pointer to the session in the lambda to prevent the session from destructing before
//         // the callback completes
//         auto self(shared_from_this());
//         socket_.async_read_some(asio::buffer(data_, max_length),
//             [this, self](std::error_code ec, std::size_t length) {
//                 if (!ec) {
//                     // Determine if we have a full command

//                     // Process the command

//                     do_write(length);
//                 } else {
//                     // log the error
//                 }
//             });
//     }

//     void do_write(std::size_t length) {
//         auto self(shared_from_this());
//         asio::async_write(socket_, asio::buffer(data_, length),
//             [this, self](std::error_code ec, std::size_t /*length*/) {
//                 if (!ec) {
//                     do_read();
//                 }
//             });
//     }
// };

// class TcpServer {
// public:
//     TcpServer(asio::io_context& io_context, short port) 
//         : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
//         do_accept();
//     }

// private:
//     asio::ip::tcp::acceptor acceptor_;

//     void do_accept() {
//         acceptor_.async_accept(
//             [this](std::error_code ec, asio::ip::tcp::socket socket) {
//                 if (!ec) {
//                     std::cout << "Starting session" << '\n';
//                     std::make_shared<Session>(std::move(socket))->start();
//                 }
//                 std::cout << "Session ending, looking for new connection" << std::endl;
//                 do_accept();
//             });
//     }
// };


int main() {
    try {
        asio::io_context io_context;

        // Need to give io_context work before calling run
        TcpServer server(io_context, 13);

        std::vector<std::thread> threads;
        for(int i = 0; i < std::thread::hardware_concurrency(); ++i) {
            threads.emplace_back([&io_context](){
                io_context.run();
            });
        }

        for (auto& th : threads) {
            if (th.joinable()) th.join();
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
