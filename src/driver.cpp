#include "server.h"

#include <iostream>
#include <memory>
#include <asio.hpp>


int main(int argc, char *argv[]) {
    bool help = false;
    short port = 13;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            help = true;
        } else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                port = static_cast<short>(std::stoi(argv[++i]));
            } else {
                std::cerr << "-p option requires one argument." << std::endl;
                return 1;
            }
        }
    }

    if (help) {
        std::cout << "Usage: ./program [-p port] [-h]" << std::endl;
        return 0;
    }

    try {
        asio::io_context io_context;

        // Need to give io_context work before calling run
        TcpServer server(io_context, port);
        std::cout << "Server starting up on port: " << port << std::endl;

        std::vector<std::thread> threads;
        // TODO: Enable multithreading (requires synchronization)
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
