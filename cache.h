#ifndef CACHE_H
#define CACHE_H

#include <memory>

#include "session.h"


enum Status {
    UNINITIALIZED,
    INITIALIZED,
    TRAINING,
    READY
};

enum Commands {
    INITIALIZE,
    TRAIN,
    LOAD,
    SEARCH
};

class Cache {
public:
    Cache();
    void processCommand(std::shared_ptr<Session> session, std::string command);
    ~Cache();

private:
    Status status;

};


#endif