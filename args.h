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

    const static size_t static_size;
    size_t size;

    virtual size_t get_static_size() = 0;
    virtual Command get_command() = 0;
    virtual void deserialize(std::istream& is) = 0;
    virtual void deserialize_static(std::istream& is) = 0;
    virtual void deserialize_dynamic(std::istream& is) = 0;
    virtual ~Args() {}

protected:
    template<typename T>
    std::shared_ptr<T[]> read_dynamic_data(std::istream& is, size_t size) {
        std::shared_ptr<T[]> data(new T[size]);
        for (size_t i = 0; i < size; i++) {
            this->read_arg<T>(&data[i], is);
        }
        return data;
    }

    template<typename T>
    void read_arg(T *arg, std::istream& is) {
        char buffer[sizeof(T)];
        is.read(buffer, sizeof(T));
        if (!is) {
            // std::cerr << "Failed to read the required number of bytes." << std::endl;
            return;
        }
        memcpy(arg, buffer, sizeof(T));
    }

    void read_end_delimiter(std::istream& is) {
        char temp;
        this->read_arg<char>(&temp, is);
        if (temp != '\r') {
            std::cerr << "Expected end delimiter but didn't find it" << std::endl;
        }
        this->read_arg<char>(&temp, is);
        if (temp != '\n') {
            std::cerr << "Expected end delimiter but didn't find it" << std::endl;
        }
    }
};

struct InitializeArgs : Args {
    size_t d;
    size_t max_mem;
    size_t nTotal;
    const static size_t static_size = 24;
    std::shared_ptr<char[]> db_url;

    virtual size_t get_static_size() override;
    virtual Command get_command() override;
    virtual void deserialize(std::istream& is) override;
    virtual void deserialize_static(std::istream& is) override;
    virtual void deserialize_dynamic(std::istream& is) override;
};


struct TrainArgs : Args {
    const static size_t static_size = 9;
    std::shared_ptr<float[]> training_data;

    virtual size_t get_static_size() override;
    virtual Command get_command() override;
    virtual void deserialize(std::istream& is) override;
    virtual void deserialize_static(std::istream& is) override;
    virtual void deserialize_dynamic(std::istream& is) override;
};

#endif
