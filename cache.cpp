#include "cache.h"

#include <iostream>
#include <memory>


Cache::Cache() : status(UNINITIALIZED) {}

// Should the command be passed in or should session.read_command be called?
void Cache::processCommand(std::shared_ptr<Session> session, std::string command) {
    // Determine the whether we can process the command
    std::string output("No message written");
    switch (this->status) {
        case UNINITIALIZED:
            if (command == std::string("INITIALIZE")) {
                output = "Processing command!";
            } else {
                output = "Cannot process command";
            }
            break;
        default:
            output = "Cache somehow in an uninitialized state";
    }

    output.copy(session->data_, 1024);
    std::cout << output << std::endl;
    session->do_write(output.size());
    session->read_command([this](std::shared_ptr<Session> session, std::string command) {
        this->processCommand(session, command);
    });
}

Cache::~Cache() {
    std::cout << "Cache destructed" << std::endl;
}