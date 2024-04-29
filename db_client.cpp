#include "db_client.h"

#include <iostream>
#include <faiss/IndexFlat.h>


DBClient::DBClient(size_t d) {
    this->d = d;
    this->size = 0;
    this->index = std::unique_ptr<faiss::Index>(new faiss::IndexFlatL2(this->d));
}

void DBClient::loadDB(faiss::idx_t n, float* data) {
    // Add data to the index
    this->index->add(n, data);
    // Save the data in the 
    this->data = std::unique_ptr<float[]>(new float[this->d * n]);
    std::memcpy(this->data.get(), data, this->d * sizeof(float) * n);
    this->size = n;
}

void DBClient::search(faiss::idx_t n, float *xq, faiss::idx_t k, float *x) {
    // TODO: GUARD AGAINST Querying greater than the size of the database
    float distances[n * k];
    faiss::idx_t labels[n * k];
    this->index->search(n, xq, k, distances, labels);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < k; j++) {
            // Copying data from 
            // Does there need to be a * this->d when indexing into lables or not?
            // Strange connection between this and which cell 0 / cell 1 working. Never both at the same time.
            std::memcpy(&x[(j + i * k) * this->d], &this->data[labels[j + i * k] * this->d], sizeof(float) * d);
        }
    }
}