#include "db_client.h"


#include <iostream>
#include <faiss/IndexFlat.h>


DBClient::DBClient(size_t d) {
    this->d = d;
    this->index = std::unique_ptr<faiss::Index>(new faiss::IndexFlatL2(this->d));
}

void DBClient::loadDB(faiss::idx_t n, float* data) {
    // Add data to the index
    this->index->add(n, data);
    // Save the data in the 
    this->data = std::unique_ptr<float[]>(new float[this->d * n]);
    std::memcpy(this->data.get(), data, sizeof(float) * n);
}

void DBClient::search(faiss::idx_t n, float *xq, faiss::idx_t k, float *x) {
    float distances[n * k];
    faiss::idx_t labels[n * k];
    std::cout << "Search the index" << std::endl;
    this->index->search(n, xq, k, distances, labels);
    std::cout << "Got results from the index" << std::endl;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < k; j++) {
            std::memcpy(&x[(j + i * k) * this->d], &this->data[labels[j + i * k]], sizeof(float) * d);
        }
    }
}