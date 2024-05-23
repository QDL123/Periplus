#include "server.h"

#include <iostream>
#include <memory>
#include <asio.hpp>


int main() {
    try {
        asio::io_context io_context;

        // Need to give io_context work before calling run
        TcpServer server(io_context, 13);
        std::cout << "Server starting up!" << std::endl;

        std::vector<std::thread> threads;
        // TODO: Add multithreading
        for(int i = 0; i < 1 /* std::thread::hardware_concurrency()*/; ++i) {
            threads.emplace_back([&io_context](){
                io_context.run();
            });
        }

        for (auto& th : threads) {
            if (th.joinable()) th.join();
        }

        std::cout << "Threads joined!" << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
