#include "cache.h"
#include "args.h"
#include "session.h"

#include <iostream>
#include <math.h>
#include <memory>


Cache::Cache() : status(UNINITIALIZED), core(nullptr) {}

// Should the command be passed in or should session.read_command be called?
void Cache::processCommand(std::shared_ptr<Session> session, std::string command) {
    // Determine the whether we can process the command
    std::cout << "Received command: " << command << std::endl;
    std::string output("No message wr itten");
    switch (this->status) {
        case UNINITIALIZED:
            if (command == std::string("INITIALIZE")) {
                output = "Processing initialize command!";
                std::shared_ptr<InitializeArgs> args = std::make_shared<InitializeArgs>();
                session->read_args(args);
            } else {
                output = std::string("Cannot process ") + command + std::string(" command");
            }
            break;
        default:
            output = "Cache somehow in an initialized state";
            break;
    }
    std::cout << "Received command: " << command << std::endl;
    
    // Send back update that the command is being processed
    output.copy(session->data_, 1024);
    std::cout << output << std::endl;
}


void Cache::process_args(std::shared_ptr<Session> session) {
    // Complete any logic which is command agnostic
    // We now have a completed args object
    // Determine the command
    if (session->args->get_command() == INITIALIZE) {
        std::cout << "Starting initialization" << std::endl;
        this->initialize(session);
    }
}

size_t Cache::determineNCells(size_t nTotal) {
    // TODO: modify this to account for low total
    return 4 * sqrt(nTotal);
}


void Cache::initialize(std::shared_ptr<Session> session) {
    // TODO: create DB client
    std::cout << "Creating the db client" << std::endl;
    std::shared_ptr<InitializeArgs> args = std::dynamic_pointer_cast<InitializeArgs>(session->args);

    std::shared_ptr<DBClient> db_client = std::make_shared<DBClient>(args->d);

    // Calculate nCells
    size_t nCells = determineNCells(args->nTotal);

    this->core = std::make_unique<Core>(args->d, db_client, nCells, args->nTotal);

    std::string output("Initialized cache");
    output.copy(session->data_, 1024);

    session->do_write(output.size());

    std::cout << "Setting state to INITIALIZED" << std::endl;
    this->status = INITIALIZED;

    std::cout << "Listening for more commands" << std::endl;
    // Listen for more commands
    session->read_command();

    // Last reference should args should go out of scope, causing it to destruct
}

Cache::~Cache() {
    std::cout << "Cache destructed" << std::endl;
}