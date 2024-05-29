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
    std::string output("No message written");
    std::cout << "Received command: " << command << std::endl;
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
        case INITIALIZED:
            if (command == std::string("INITIALIZE")) {
                output = "Processing initialize command!";
                std::shared_ptr<InitializeArgs> args = std::make_shared<InitializeArgs>();
                session->read_args(args);
            } else if (command == std::string("TRAIN")) {
                output = "Processing train command!";
                std::shared_ptr<TrainArgs> args = std::make_shared<TrainArgs>();
                // session->read_args(args);
                session->read_static_args(args);
            } else {
                output = std::string("Cannot process ") + command + std::string(" command");
            }
        default:
            output = "Cache somehow in an initialized state";
            break;
    }
    
    // Send back update that the command is being processed
    output.copy(session->data_, 1024);
}


void Cache::process_args(std::shared_ptr<Session> session) {
    // Complete any logic which is command agnostic
    // We now have a completed args object
    // Determine the command
    if (session->args->get_command() == INITIALIZE) {
        std::cout << "Starting initialization" << std::endl;
        this->initialize(session);
    } else if (session->args->get_command() == TRAIN) {
        std::cout << "Starting training" << std::endl;
        this->train(session);
    }
}

size_t Cache::determineNCells(size_t nTotal) {
    // TODO: modify this to account for low total
    return 4 * sqrt(nTotal);
}


void Cache::initialize(std::shared_ptr<Session> session) {
    // TODO: create DB client
    std::shared_ptr<InitializeArgs> args = std::dynamic_pointer_cast<InitializeArgs>(session->args);

    std::shared_ptr<DBClient> db_client = std::make_shared<DBClient>(args->d);

    // Calculate nCells
    size_t nCells = determineNCells(args->nTotal);

    this->core = std::make_unique<Core>(args->d, db_client, nCells, args->nTotal);

    std::string output("Initialized cache");
    output.copy(session->data_, 1024);

    session->do_write(output.size());

    this->status = INITIALIZED;

    std::cout << "Listening for more commands" << std::endl;
    // Listen for more commands
    session->read_command();

    // Last reference should args should go out of scope, causing it to destruct
}

void Cache::train(std::shared_ptr<Session> session) {
    std::shared_ptr<TrainArgs> args = std::dynamic_pointer_cast<TrainArgs>(session->args);

    faiss::idx_t nTrainingVecs = (faiss::idx_t)args->size / sizeof(float) / this->core->d;
    // TODO: Make this async so the server can respond while the core is training.
    this->core->train(nTrainingVecs, args->training_data.get());
    assert(this->core->index->is_trained);
    this->status = READY;

    std::string output("Trained cache");
    output.copy(session->data_, 1024);

    session->do_write(output.size());

    // Listen for more commands
    session->read_command();
}

Cache::~Cache() {
    std::cout << "Cache destructed" << std::endl;
}