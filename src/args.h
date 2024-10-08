/*
The goal here should be to abstract the network serialization protocol from sessions and from the cache.
The cache will of course know how to handle commands, but nothing about serialization or how the commands
are transmitted over the wire. The session knows nothing about the commands it is parsing. It uses the arg
structs passed into it to read commands.
*/

#ifndef ARGS_H
#define ARGS_H

#include <memory>
#include <vector>
#include <cstring>
#include <iostream>

enum Command {
    INITIALIZE,
    TRAIN,
    LOAD,
    SEARCH,
    EVICT,
    ADD
};

struct Args {
    size_t size;
    size_t static_size;

    virtual size_t get_static_size() { return static_size; };
    virtual Command get_command() = 0;
    virtual void deserialize_static(std::istream& is) = 0;
    virtual void deserialize_dynamic(std::istream& is) = 0;
    virtual ~Args() {}

protected:
    template<typename T>
    bool isChar() {
        if constexpr(std::is_same<T, char>::value)
            return true;
        return false;
    }

    template<typename T>
    std::shared_ptr<T[]> read_dynamic_data(std::istream& is, size_t size) {
        if (isChar<T>()) {
            std::shared_ptr<T[]> data(new T[size + 1]);
            for (size_t i = 0; i < size; i++) {
                this->read_arg<T>(&data[i], is);
            }
            data[size] = '\0'; // Add null terminator if the data is a char
            return data;
        }
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
            std::cerr << "Failed to read the required number of bytes." << std::endl;
            throw;
            // return;
        }
        memcpy(arg, buffer, sizeof(T));
    }

    void read_static_delimiter(std::istream& is) {
        char temp;
        this->read_arg<char>(&temp, is);
        if (temp != '\n') {
            std::cerr << "Expected static delimiter (\\n) but didn't find it" << std::endl;
            // TODO: somehow abandon command (Probably throw an error that gets caught in the cache layer)
        }
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
    bool use_flat;
    const static size_t static_size = 4 * sizeof(size_t) + sizeof(bool) + sizeof(char);
    std::shared_ptr<char[]> db_url;

    virtual size_t get_static_size() override { return static_size; };
    virtual Command get_command() override { return INITIALIZE; };
    virtual void deserialize_static(std::istream& is) override;
    virtual void deserialize_dynamic(std::istream& is) override;
};


struct TrainArgs : Args {
    const static size_t static_size = 9;
    std::shared_ptr<float[]> training_data;

    virtual size_t get_static_size() override { return static_size; };
    virtual Command get_command() override { return TRAIN; };
    virtual void deserialize_static(std::istream& is) override;
    virtual void deserialize_dynamic(std::istream& is) override;
};


struct LoadArgs : Args {
    const static size_t static_size = 2 * sizeof(size_t) + sizeof(char);
    size_t nload;
    std::shared_ptr<float[]> xq;
    
    virtual size_t get_static_size() override { return static_size; }
    virtual Command get_command() override { return LOAD; };
    virtual void deserialize_static(std::istream& is) override;
    virtual void deserialize_dynamic(std::istream& is) override;
};

struct SearchArgs : Args {
    const static size_t static_size = 4 * sizeof(size_t) + sizeof(char) + sizeof(bool);
    size_t n;
    size_t k;
    size_t nprobe;
    bool require_all;
    std::shared_ptr<float[]> xq;

    virtual size_t get_static_size() override { return static_size; }
    virtual Command get_command() override { return SEARCH; }
    virtual void deserialize_static(std::istream& is) override;
    virtual void deserialize_dynamic(std::istream& is) override;
};

struct EvictArgs : Args {
    const static size_t static_size = 2 * sizeof(size_t) + sizeof(char);
    size_t nevict;
    std::shared_ptr<float[]> xq;

    virtual size_t get_static_size() override { return static_size; }
    virtual Command get_command() override { return EVICT; }
    virtual void deserialize_static(std::istream& is) override;
    virtual void deserialize_dynamic(std::istream& is) override;
};

// Add?, Track?, Register?, Notify?
struct AddArgs : Args {
    const static size_t static_size = sizeof(size_t) + sizeof(char);
    size_t num_docs;

    std::shared_ptr<float[]> embeddings;
    std::vector<std::shared_ptr<char[]>> ids;

    virtual size_t get_static_size() override { return static_size; }
    virtual Command get_command() override { return ADD; }
    virtual void deserialize_static(std::istream& is) override;
    virtual void deserialize_dynamic(std::istream& is ) override;
};

#endif
