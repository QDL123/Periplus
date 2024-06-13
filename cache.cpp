#include "cache.h"
#include "args.h"
#include "session.h"

#include <random>
#include <iostream>
#include <math.h>
#include <memory>


Cache::Cache() : status(UNINITIALIZED), core(nullptr) {}

// Should the command be passed in or should session.read_command be called?
void Cache::processCommand(std::shared_ptr<Session> session, std::string command) {
    // Determine the whether we can process the command
    std::string output("Unable to process command: " + command);
    std::cout << "Received command: " << command << " with status: " << this->status << std::endl;
    switch (this->status) {
        case READY:
            if (command == std::string("SEARCH")) {
                output = "Processing search command";
                std::shared_ptr<SearchArgs> args = std::make_shared<SearchArgs>();
                std::cout << "Calling read_static_args" << std::endl;
                session->updated_read_args(args);
                // session->read_static_args(args);
                // session->read_args(args);
                break;
            } else if (command == std::string("LOAD")) {
                output = "Processing load command!";
                std::shared_ptr<LoadArgs> args = std::make_shared<LoadArgs>();
                session->read_args(args);
                break;
            } else {
                std::cout << "DIDn't match READY command" << std::endl;
            }
        case INITIALIZED:
            if (command == std::string("TRAIN")){
                output = "Processing train command!";
                std::shared_ptr<TrainArgs> args = std::make_shared<TrainArgs>();
                // session->read_args(args);
                // session->read_static_args(args);
                session->updated_read_args(args);
                break;
            }
        case UNINITIALIZED:
            if (command == std::string("INITIALIZE")) {
                output = "Processing initialize command!";
                std::shared_ptr<InitializeArgs> args = std::make_shared<InitializeArgs>();
                session->read_args(args);
                break;
            }
        default:
            std::cout << "Cache status: " << this->status << std::endl;
            throw std::runtime_error(std::string("Cache status is undefined"));
    }
    
    // Send back update that the command is being processed
    output.copy(session->data_, 1024);
}


void Cache::process_args(std::shared_ptr<Session> session) {
    // Complete any logic which is command agnostic
    // We now have a completed args object
    // Determine the command
    if (session->args->get_command() == SEARCH) {
        std::cout << "Starting search" << std::endl;
        this->search(session);
    } else if (session->args->get_command() == INITIALIZE) {
        std::cout << "Starting initialization" << std::endl;
        this->initialize(session);
    } else if (session->args->get_command() == TRAIN) {
        std::cout << "Starting training" << std::endl;
        this->train(session);
    } else if (session->args->get_command() == LOAD) {
        std::cout << "Starting load" << std::endl;
        this->load(session);
    }
}

size_t Cache::determineNCells(size_t nTotal) {
    // TODO: modify this to account for low total
    return 4 * sqrt(nTotal);
}


void Cache::initialize(std::shared_ptr<Session> session) {
    // TODO: create DB client
    std::shared_ptr<InitializeArgs> args = std::dynamic_pointer_cast<InitializeArgs>(session->args);

    std::shared_ptr<DBClient> db_client = std::make_shared<DBClient>(args->d, args->db_url);
    // std::shared_ptr<DBClient> db_client = std::make_shared<DBClient>(args->d);

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

    // TODO: REMOVE THIS ////////////////////////////////
    // Load the mock client manually in the test file, otherwise use the actual client with no load function
    // std::vector<Data> dataset;
    // std::cout << "generating dataset" << std::endl;
    // for (size_t i = 0; i < nTrainingVecs; i++) {
    //     char id[2];
    //     id[0] = 'i';
    //     id[1] = 'd';
        
    //     float embedding[this->core->d];

    //     for (size_t j = 0; j < this->core->d; j++) {
    //         embedding[j] = args->training_data[i * this->core->d + j];
    //     }

    //     char document[3];
    //     document[0] = 'd';
    //     document[1] = 'o';
    //     document[2] = 'c';

    //     char metadata[4];
    //     metadata[0] = 'm';
    //     metadata[1] = 'e';
    //     metadata[2] = 't';
    //     metadata[3] = 'a';

    //     // std::cout << "About to construct data struct" << std::endl;
    //     dataset.push_back(Data(2, this->core->d, 3, 4, id, embedding, document, metadata));
    // }
    // std::cout << "loading db" << std::endl;

    // this->core->db->loadDB(nTrainingVecs, dataset.data());
    // std::cout << "Finished loading db" << std::endl;
    /////////////////////////////////////////////////////

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

void Cache::load(std::shared_ptr<Session> session) {
    std::shared_ptr<LoadArgs> args = std::dynamic_pointer_cast<LoadArgs>(session->args);
    this->core->loadCellWithVec(args->xq);
    
    std::string output("Loaded cell");
    output.copy(session->data_, 1024);
    session->do_write(output.size());

    // Listen for more commands
    session->read_command();
}

void Cache::search(std::shared_ptr<Session> session) {
    std::shared_ptr<SearchArgs> args = std::dynamic_pointer_cast<SearchArgs>(session->args);
    Data results[args->n * args->k];
    int cacheHits[args->n];

    this->core->search(args->n, args->xq.get(), args->k, results, cacheHits);

    std::cout << "n: " << args->n << ", k: " << args->k << " , size: " << args->size << std::endl;
    for (size_t i = 0; i < args->n; i++) {
        std::memcpy(session->data_, &cacheHits[i], sizeof(int));
        session->sync_write(sizeof(int));
        for (size_t j = 0; j < cacheHits[i]; j++) {
            std::vector<char> bytes;
            results[(i * args->k) + j].serialize(bytes);
            size_t l = 0;

            for (; bytes.size() >= Session::max_length && l < bytes.size() - Session::max_length; l += Session::max_length) {
                std::memcpy(session->data_, &bytes.data()[l], Session::max_length);
                session->sync_write(Session::max_length);
            }

            std::memcpy(session->data_, &bytes.data()[l], bytes.size() - l);
            session->sync_write(bytes.size() - l);
        }
    }
}

Cache::~Cache() {
    std::cout << "Cache destructed" << std::endl;
}