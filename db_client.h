#ifndef DB_CLIENT_H
#define DB_CLIENT_H

#include "data.h"
#include <faiss/IndexFlat.h>

struct DBClient {
    size_t d;
    size_t size;
    std::unique_ptr<faiss::Index> index;
    // std::unique_ptr<float[]> data;
    std::unique_ptr<Data[]> data;

    DBClient(size_t d);

    void loadDB(faiss::idx_t n, Data *data);


    // TODO: this function should be responsible for creating data structs 
    void search(faiss::idx_t n, float *xq, faiss::idx_t k, Data *x);
};

#endif