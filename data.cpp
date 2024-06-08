#include "data.h"
#include <iostream>
#include <cstring>


Data::Data()
    : id_len(0), embedding_len(0), document_len(0), metadata_len(0), id(nullptr), embedding(nullptr), document(nullptr), metadata(nullptr) {}


// Data::Data(std::istream& is) {
//     // TODO: implement deserialization   
// }

// Copy Constructor
Data::Data(const Data& other) : id_len(other.id_len), embedding_len(other.embedding_len), document_len(other.document_len), metadata_len(other.metadata_len),
    id(other.id), embedding(other.embedding), document(other.document), metadata(other.metadata)  {}

// Move Constructor
Data::Data(Data&& other) noexcept : id_len(other.id_len), embedding_len(other.embedding_len), document_len(other.document_len), metadata_len(other.metadata_len),
    id(std::move(other.id)), embedding(std::move(other.embedding)), document(std::move(other.document)), metadata(std::move(other.metadata)) {}


Data::Data(size_t id_len, size_t embedding_len, size_t document_len, size_t metadata_len, char *id, float *embedding, char *document, char *metadata)
    : id_len(id_len), embedding_len(embedding_len), document_len(document_len), metadata_len(metadata_len) {

        this->id = std::shared_ptr<char[]>(new char[id_len]);
        this->embedding = std::shared_ptr<float[]>(new float[embedding_len]);
        this->document = std::shared_ptr<char[]>(new char[document_len]);
        this->metadata = std::shared_ptr<char[]>(new char[metadata_len]);

        std::memcpy(this->id.get(), id, id_len);
        std::memcpy(this->embedding.get(), embedding, sizeof(float) * embedding_len);
        std::memcpy(this->document.get(), document, document_len);
        std::memcpy(this->metadata.get(), metadata, metadata_len);
 }


 Data& Data::operator=(const Data& other) {
    if (this != &other) {
        this->id_len = other.id_len;
        this->embedding_len = other.embedding_len;
        this->document_len = other.document_len;
        this->metadata_len = other.metadata_len;

        this->id = other.id;
        this->embedding = other.embedding;
        this->document = other.document;
        this->metadata = other.metadata;
    }
    return *this;
}

// Move Assignment Operator
Data& Data::operator=(Data&& other) noexcept {
    if (this != &other) {
        this->id_len = other.id_len;
        this->embedding_len = other.embedding_len;
        this->document_len = other.document_len;
        this->metadata_len = other.metadata_len;

        this->id = std::move(other.id);
        this->embedding = std::move(other.embedding);
        this->document = std::move(other.document);
        this->metadata = std::move(other.metadata);
    }
    return *this;
}

void Data::serialize(std::vector<char>& bytes) {
    size_t vectorSize = 4 * sizeof(size_t) + this->id_len + this->document_len + this->metadata_len + sizeof(float) * this->embedding_len;
    bytes.resize(vectorSize);

    size_t index = 0;
    // Send the id
    std::memcpy(bytes.data(), &this->id_len, sizeof(this->id_len));
    index += sizeof(this->id_len);
    std::memcpy(&bytes.data()[index], this->id.get(), this->id_len);
    index += this->id_len;

    // Send the embedding
    std::memcpy(&bytes.data()[index], &this->embedding_len, sizeof(this->embedding_len));
    index += sizeof(this->embedding_len);
    std::memcpy(&bytes.data()[index], this->embedding.get(), sizeof(float) * this->embedding_len);
    index += this->embedding_len * sizeof(float);

    // Send the document
    std::memcpy(&bytes.data()[index], &this->document_len, sizeof(this->document_len));
    index += sizeof(this->document_len);
    std::memcpy(&bytes.data()[index], this->document.get(), this->document_len);
    index += this->document_len;

    // Send the metadata
    std::memcpy(&bytes.data()[index], &this->metadata_len, sizeof(this->metadata_len));
    index += sizeof(this->metadata_len);
    std::memcpy(&bytes.data()[index], this->metadata.get(), this->metadata_len);
}


Data::~Data() {}