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
            if (i + 1 < argc) {  // Make sure we aren't at the end of argv!
                port = static_cast<short>(std::stoi(argv[++i]));  // Increment 'i' so we don't get the argument as the next argv[i].
            } else {  // Uh-oh, there was no argument to the filename option.
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
        std::cout << "sizeof(float): " << sizeof(float) << std::endl;

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
