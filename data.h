#ifndef DATA_H
#define DATA_H

#include <cstddef>
#include <iostream>

struct Data {

    size_t id_len;
    size_t embedding_len;
    size_t document_len;
    size_t metadata_len;

    // char *id;
    // float *embedding;
    // char *document;
    // char *metadata;

    std::shared_ptr<char[]> id;
    std::shared_ptr<float[]> embedding;
    std::shared_ptr<char[]> document;
    std::shared_ptr<char[]> metadata;


    // Used for deserializing data coming from db
    Data();

    Data(const Data& other);

    Data(Data&& other) noexcept;

    // Data(std::istream& is);
    Data(size_t id_len, size_t embedding_len, size_t document_len, size_t metadata_len,
        char *id, float *embedding, char *document, char *metadata);

    Data& operator=(const Data& other);
    Data& operator=(Data&& other) noexcept;

    void serialize(std::vector<char>& bytes);

    ~Data();
};

#endif