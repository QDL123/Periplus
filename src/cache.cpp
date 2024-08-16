#include "cache.h"
#include "args.h"
#include "session.h"
#include "exceptions.h"

#include <random>
#include <iostream>
#include <math.h>
#include <memory>


Cache::Cache() : status(UNINITIALIZED), core(nullptr) {}

// Should the command be passed in or should session.read_command be called?
void Cache::processCommand(std::shared_ptr<Session> session, std::string command) {
    // Determine the whether we can process the command
    std::string output("Unable to process command: " + command);
    std::cout << "Received command: " << command << '\n';
    switch (this->status) {
        case READY:
            if (command == std::string("SEARCH")) {
                output = "Parsing search command";
                std::shared_ptr<SearchArgs> args = std::make_shared<SearchArgs>();
                session->updated_read_args(args);
                break;
            } else if (command == std::string("ADD")) {
                output = "Parsing add command";
                std::shared_ptr<AddArgs> args = std::make_shared<AddArgs>();
                session->updated_read_args(args);
                break;
            } else if (command == std::string("LOAD")) {
                output = "Parsing load command!";
                std::shared_ptr<LoadArgs> args = std::make_shared<LoadArgs>();
                session->updated_read_args(args);
                break;
            } else if (command == std::string("EVICT")) {
                output = "Parsing evict command!";
                std::shared_ptr<EvictArgs> args = std::make_shared<EvictArgs>();
                session->updated_read_args(args);
                break;
            } else {
                std::cout << "DIDn't match READY command" << std::endl;
            }
        case INITIALIZED:
            if (command == std::string("TRAIN")){
                output = "Parsing train command!";
                std::shared_ptr<TrainArgs> args = std::make_shared<TrainArgs>();
                session->updated_read_args(args);
                break;
            }
        case UNINITIALIZED:
            if (command == std::string("INITIALIZE")) {
                output = "Processing initialize command!";
                std::shared_ptr<InitializeArgs> args = std::make_shared<InitializeArgs>();
                session->updated_read_args(args);
                break;
            }
        default:
            std::cout << "Could not process command: " << command << " with cache status: " << this->status << std::endl;
            throw std::runtime_error(std::string("Invalid command"));
    }
}


void Cache::process_args(std::shared_ptr<Session> session) {
    // Complete any logic which is command agnostic
    // We now have a completed args object
    // Determine the command
    if (session->args->get_command() == SEARCH) {
        std::cout << "Performing search" << std::endl;
        this->search(session);
    } else if (session->args->get_command() == ADD) {
        std::cout << "Performing add" << std::endl;
        this->add(session);
    } else if (session->args->get_command() == INITIALIZE) {
        std::cout << "Performing initialization" << std::endl;
        this->initialize(session);
    } else if (session->args->get_command() == TRAIN) {
        std::cout << "Performing training" << std::endl;
        this->train(session);
    } else if (session->args->get_command() == LOAD) {
        std::cout << "Performing load" << std::endl;
        this->load(session);
    } else if (session->args->get_command() == EVICT) {
        std::cout << "Performing evict" << std::endl;
        this->evict(session);
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

    // Calculate nCells
    size_t nCells = determineNCells(args->nTotal);
    std::cout << "nCells: " << nCells << std::endl;

    this->core = std::make_unique<Core>(args->d, db_client, nCells, args->nTotal, args->use_flat);

    std::string output("Initialized cache");
    output.copy(session->output_buf, 1024);

    session->do_write(output.size());

    this->status = INITIALIZED;

    std::cout << "Listening for more commands" << std::endl;
    // Listen for more commands
    session->read_command();
}

void Cache::train(std::shared_ptr<Session> session) {
    std::shared_ptr<TrainArgs> args = std::dynamic_pointer_cast<TrainArgs>(session->args);
    faiss::idx_t nTrainingVecs = (faiss::idx_t)args->size / sizeof(float) / this->core->d;

    // TODO: Make this async so the server can respond while the core is training.
    this->core->train(nTrainingVecs, args->training_data.get());
    assert(this->core->index->is_trained);
    this->status = READY;

    std::string output("Trained cache");
    output.copy(session->output_buf, 1024);

    session->do_write(output.size());

    // Listen for more commands
    session->read_command();
}

void Cache::load(std::shared_ptr<Session> session) {
    std::shared_ptr<LoadArgs> args = std::dynamic_pointer_cast<LoadArgs>(session->args);
    std::string output("Loaded cell");
    try {
        this->core->loadCellWithVec(args->xq, args->nload);
    } catch (const HttpException& e) {
        output = e.what();
    }
    output.copy(session->output_buf, 1024);
    session->do_write(output.size());

    // Listen for more commands
    session->read_command();
}

void Cache::search(std::shared_ptr<Session> session) {
    std::shared_ptr<SearchArgs> args = std::dynamic_pointer_cast<SearchArgs>(session->args);
    Data results[args->n * args->k];
    int cacheHits[args->n];

    bool require_all = true;
    this->core->search(args->n, args->xq.get(), args->k, args->nprobe, args->require_all, results, cacheHits);
    std::cout << "cacheHits: ";
    for (size_t i = 0; i < args->n; i++) {
        std::cout << cacheHits[i];
        if (i < args->n - 1) {
            std::cout << ", ";
        }
    }
    std::cout << '\n';

    for (size_t i = 0; i < args->n; i++) {
        std::memcpy(session->output_buf, &cacheHits[i], sizeof(int));
        session->sync_write(sizeof(int));
        for (int j = 0; cacheHits[i] > -1 && j < cacheHits[i]; j++) {
            std::vector<char> bytes;
            results[(i * args->k) + j].serialize(bytes);
            size_t l = 0;

            for (; bytes.size() >= Session::max_length && l < bytes.size() - Session::max_length; l += Session::max_length) {
                std::memcpy(session->output_buf, &bytes.data()[l], Session::max_length);
                session->sync_write(Session::max_length);
            }

            std::memcpy(session->output_buf, &bytes.data()[l], bytes.size() - l);
            session->sync_write(bytes.size() - l);
        }
    }

    session->read_command();
}

void Cache::evict(std::shared_ptr<Session> session) {
    std::shared_ptr<EvictArgs> args = std::dynamic_pointer_cast<EvictArgs>(session->args);
    this->core->evictCellWithVec(args->xq, args->nevict);
    
    std::string output("Evicted cell");
    output.copy(session->output_buf, 1024);
    session->do_write(output.size());

    // Listen for more commands
    session->read_command();
}

void Cache::add(std::shared_ptr<Session> session) {
    std::shared_ptr<AddArgs> args = std::dynamic_pointer_cast<AddArgs>(session->args);
    this->core->add(args->num_docs, args->ids, args->embeddings);

    std::string output("Added vectors");
    output.copy(session->output_buf, 1024);
    session->do_write(output.size());
    session->read_command();
}

Cache::~Cache() {
    std::cout << "Cache destructed" << std::endl;
}