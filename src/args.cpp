#include "args.h"

#include <iostream>
#include <memory>

void InitializeArgs::deserialize_static(std::istream& is) {
    this->read_arg<size_t>(&this->d, is);
    this->read_arg<size_t>(&this->max_mem, is);
    this->read_arg<size_t>(&this->nTotal, is);
    this->read_arg<bool>(&this->use_flat, is);
    this->read_arg<size_t>(&this->size, is);

    this->read_static_delimiter(is);
}

void InitializeArgs::deserialize_dynamic(std::istream& is) {
    this->db_url = this->read_dynamic_data<char>(is, this->size);
    this->read_end_delimiter(is);
}


void TrainArgs::deserialize_static(std::istream& is) {
    this->read_arg<size_t>(&this->size, is);
    this->read_static_delimiter(is);
}

void TrainArgs::deserialize_dynamic(std::istream& is) {
    size_t nFloats = this->size / sizeof(float);
    this->training_data = this->read_dynamic_data<float>(is, nFloats);
    this->read_end_delimiter(is);
}

void LoadArgs::deserialize_static(std::istream& is) {
    this->read_arg<size_t>(&this->nload, is);
    this->read_arg<size_t>(&this->size, is);
    this->read_static_delimiter(is);
}

void LoadArgs::deserialize_dynamic(std::istream& is) {
    size_t nFloats = this->size / sizeof(float);
    this->xq = this->read_dynamic_data<float>(is, nFloats);

    this->read_end_delimiter(is);
}


void SearchArgs::deserialize_static(std::istream& is) {
    this->read_arg<size_t>(&this->n, is);
    this->read_arg<size_t>(&this->k, is);
    this->read_arg<size_t>(&this->nprobe, is);
    this->read_arg<bool>(&this->require_all, is);
    this->read_arg<size_t>(&this->size, is);
    this->read_static_delimiter(is);
}

void SearchArgs::deserialize_dynamic(std::istream& is) {
    size_t nFloats = this->size / sizeof(float);
    this->xq = this->read_dynamic_data<float>(is, nFloats);
    this->read_end_delimiter(is);
}

void EvictArgs::deserialize_static(std::istream& is) {
    this->read_arg<size_t>(&this->nevict, is);
    this->read_arg<size_t>(&this->size, is);
    this->read_static_delimiter(is);
}

void EvictArgs::deserialize_dynamic(std::istream& is) {
    size_t nFloats = this->size / sizeof(float);
    this->xq = this->read_dynamic_data<float>(is, nFloats);
    this->read_end_delimiter(is);
}

void AddArgs::deserialize_static(std::istream& is) {
    this->read_arg<size_t>(&this->num_docs, is);
    this->read_arg<size_t>(&this->size, is);
    this->read_static_delimiter(is);
}

void AddArgs::deserialize_dynamic(std::istream& is) {
    size_t totalSize = 0;
    for (size_t i = 0; i < this->num_docs; i++) {
        size_t id_len;
        this->read_arg<size_t>(&id_len, is);
        totalSize += id_len;
        totalSize += sizeof(id_len);
        std::shared_ptr<char[]> id = this->read_dynamic_data<char>(is, id_len);
        this->ids.push_back(id);
    }

    // Use delimiter between the ids and the embeddings
    this->read_static_delimiter(is);
    totalSize += 1;
    
    size_t num_floats = (this->size - totalSize) / sizeof(float);
    this->embeddings = this->read_dynamic_data<float>(is, num_floats);
    this->read_end_delimiter(is);
}