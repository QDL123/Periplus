#ifndef CORE_H
#define CORE_H

#include "db_client.h"
#include "data.h"

#include <memory>

#include <faiss/IndexIVF.h>
#include <faiss/IndexFlat.h>
#include <faiss/IndexIDMap.h>

struct Core {

    static constexpr const double nGuessCoeff = 2;
    static constexpr const double guessScalar = 2;

    float nTotal = 0;
    size_t d = 0;
    size_t nCells = 0;
    std::unique_ptr<float[]> centroids;
    std::unique_ptr<float[]> residenceStatuses;
    std::shared_ptr<faiss::IndexFlat> quantizer;
    std::unique_ptr<faiss::IndexIVF> index;
    std::shared_ptr<DBClient> db;    

    // size of nx * d 
    // std::vector< std::vector <float> > embeddings;
    // TODO: look into other ways to generate ids;
    faiss::idx_t next_id = 0;
    std::vector<std::vector<std::string>> ids_by_cell;
    std::vector<Data> data;
    std::unordered_map<faiss::idx_t, Data> data_map;
    std::unordered_map<std::string, faiss::idx_t> id_map;
    

    Core(size_t d, std::shared_ptr<DBClient> db, size_t nCells, float nTotal, bool use_flat);
    bool isNullTerminated(const char* str, size_t maxLength);

    // May not need this is we're training the 
    // whole dataset at once. This may just be a wrapper
    void train(faiss::idx_t n, const float* x);

    void loadCellWithVec(std::shared_ptr<float[]> xq, size_t nload);

    void evictCellWithVec(std::shared_ptr<float[]> xq, size_t nevict);
    
    void loadCell(faiss::idx_t centroidIndex);

    void search(size_t n, float *xq, size_t k, size_t nprobe, bool require_all, Data *data, int *cacheHits);

    void evictCell(faiss::idx_t centroidIndex);

    void add(size_t num_docs, std::vector<std::shared_ptr<char[]>>& ids, std::shared_ptr<float[]> embeddings);


    ~Core();
};


#endif