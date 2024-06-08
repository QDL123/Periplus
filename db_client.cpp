#include "db_client.h"
#include "data.h" 

#include <iostream>
#include <faiss/IndexFlat.h>


DBClient::DBClient(size_t d) {
    this->d = d;
    this->size = 0;
    this->index = std::unique_ptr<faiss::Index>(new faiss::IndexFlatL2(this->d));
}

void DBClient::loadDB(faiss::idx_t n, Data *data) {
    // Add data to the index 
    float *embeddings = new float[n * this->d];
    for (size_t i = 0; i < n; i++) {
        std::memcpy(&embeddings[this->d * i], data[i].embedding.get(), sizeof(float) * this->d);

    }

    this->index->add(n, embeddings);
    delete[] embeddings;
    // Save the data in the 
    this->data = std::unique_ptr<Data[]>(new Data[n]);
    for (int i = 0; i < n; i++) {
        this->data[i] = std::move(data[i]);
    }
    this->size = n;
}

void DBClient::search(faiss::idx_t n, float *xq, faiss::idx_t k, Data *x) {
    // TODO: GUARD AGAINST Querying greater than the size of the database
    float distances[n * k];
    faiss::idx_t labels[n * k];
    this->index->search(n, xq, k, distances, labels);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < k; j++) {
            faiss::idx_t index = labels[j + i * k];
            Data copiedStruct(this->data[index]);
            x[j + i * k] = Data(this->data[labels[j + i * k]]);
        }
    }
}