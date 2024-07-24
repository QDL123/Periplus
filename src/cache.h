/*
The cache object is responsible for processing command and maintaing the state of the cache.
It knows nothing about the network / server implementation and nothing about the index implemented in
the Core. It is aware that there are cells which are evicted and loaded and read from. 
*/

#ifndef CACHE_H
#define CACHE_H

#include <memory>

#include "core.h"
#include "args.h"

// Forward declaration to avoid ciruclar dependencies.
class Session;


enum Status {
    UNINITIALIZED,
    INITIALIZED,
    TRAINING,
    READY
};

class Cache {
public:
    Cache();
    void processCommand(std::shared_ptr<Session> session, std::string command);
    void process_args(std::shared_ptr<Session> session);
    static size_t determineNCells(size_t nTotal); 
    void initialize(std::shared_ptr<Session> session);
    void train(std::shared_ptr<Session> session);
    void load(std::shared_ptr<Session> session);
    void search(std::shared_ptr<Session> session);
    void evict(std::shared_ptr<Session> session);
    void add(std::shared_ptr<Session> session);
    ~Cache();

private:
    Status status;
    std::unique_ptr<Core> core;
};


#endif