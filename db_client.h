#ifndef DB_CLIENT_H
#define DB_CLIENT_H

#include "data.h"
#include <faiss/IndexFlat.h>

struct DBClient {
    size_t d;
    size_t size;
    std::shared_ptr<char> db_url;

    DBClient(size_t d, std::shared_ptr<char> db_url);      
    // void loadDB(faiss::idx_t n, Data *data);
    // TODO: this function should be responsible for creating data structs 
    // This function is virtual only so it can be overidden by the mock class
    virtual void search(faiss::idx_t n, float *xq, faiss::idx_t k, Data *x);

    virtual void search(std::vector<std::string> ids, Data *x);
};


struct DBClient_Mock : DBClient {
    
    std::unique_ptr<faiss::Index> index;
    std::unique_ptr<Data[]> data;
    
    DBClient_Mock(size_t d);
    void loadDB(faiss::idx_t n, Data *data);
    // Override the search function to use the local index instead of making an API call
    void search(faiss::idx_t n, float *xq, faiss::idx_t k, Data *x) override;

    void search(std::vector<std::string> ids, Data *x) override;
};


// struct DBClient {
//     size_t d;
//     size_t size;
//     std::unique_ptr<faiss::Index> index;
//     // std::unique_ptr<float[]> data;
//     std::unique_ptr<Data[]> data;

//     DBClient(size_t d);

//     void loadDB(faiss::idx_t n, Data *data);


//     // TODO: this function should be responsible for creating data structs 
//     void search(faiss::idx_t n, float *xq, faiss::idx_t k, Data *x);
// };

#endif