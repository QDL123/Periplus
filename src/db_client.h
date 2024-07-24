#ifndef DB_CLIENT_H
#define DB_CLIENT_H

#include "data.h"
#include <faiss/IndexFlat.h>
#include <cpr/cpr.h>

struct DBClient {
    size_t d;
    size_t size;
    std::shared_ptr<char> db_url;
    cpr::Session session;

    DBClient(size_t d, std::shared_ptr<char> db_url);
    ~DBClient();      
    virtual void search(std::vector<std::string> ids, Data *x);
};


struct DBClient_Mock : DBClient {
    std::unordered_map<std::string, Data> data_map; 
    
    DBClient_Mock(size_t d);
    void loadDB(faiss::idx_t n, Data *data);
    void search(std::vector<std::string> ids, Data *x) override;
};

#endif