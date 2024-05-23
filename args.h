/*
The goal here should be to abstract the network serialization protocol from sessions and from the cache.
The cache will of course know how to handle commands, but nothing about serialization or how the commands
are transmitted over the wire. The session knows nothing about the commands it is parsing. It uses the arg
structs passed into it to read commands.
*/

#ifndef ARGS_H
#define ARGS_H

#include <iostream>

enum Command {
    INITIALIZE,
    TRAIN,
    LOAD,
    SEARCH
};

struct Args {

    size_t size;

    virtual Command get_command() = 0;
    virtual void deserialize(std::istream& is) = 0;
    virtual ~Args() {}

protected:
    template<typename T>
    std::shared_ptr<T[]> read_dynamic_data(std::istream& is, size_t size) {
        std::shared_ptr<T[]> data(new T[this->size]);
        for (size_t i = 0; i < size; i++) {
            is >> data[i];
        }
        return data;
    }

    template<typename T>
    void read_arg(T *arg, std::istream& is) {
        char buffer[sizeof(T)];
        is.read(buffer, sizeof(T));
        if (!is) {
            std::cerr << "Failed to read the required number of bytes." << std::endl;
            return;
        }
        memcpy(arg, buffer, sizeof(T));
    }
};

struct InitializeArgs : Args {
    size_t d;
    size_t max_mem;
    size_t nTotal;
    std::shared_ptr<char[]> db_url;

    virtual Command get_command() override;
    virtual void deserialize(std::istream& is) override;
};

#endif
