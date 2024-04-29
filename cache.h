#ifndef CACHE_H
#define CACHE_H

#include "db_client.h"

#include <faiss/IndexIVF.h>
#include <faiss/IndexFlat.h>
#include <faiss/IndexIDMap.h>

struct Cache {

    static constexpr const double nGuessCoeff = 2;
    static constexpr const double guessScalar = 1.5;

    float nTotal = 0;
    size_t d = 0;
    size_t nCells = 0;
    std::unique_ptr<float[]> centroids;
    std::unique_ptr<int32_t[]> residenceStatuses;
    std::shared_ptr<faiss::IndexFlat> quantizer;
    std::unique_ptr<faiss::IndexIVF> index;
    std::unique_ptr<faiss::IndexIDMap> idMap;
    std::shared_ptr<DBClient> db;    

    // size of nx * d 
    std::vector< std::vector <float> > embeddings;

    Cache(size_t d, std::shared_ptr<DBClient> db, size_t nCells, float nTotal);

    size_t getCellSize(float *x, faiss::idx_t centroidIndex, size_t prevNGuess, size_t nGuess);

    // May not need this is we're training the 
    // whole dataset at once. This may just be a wrapper
    void train(faiss::idx_t n, const float* x);

    void loadCell(faiss::idx_t centroidIndex);

    void search(size_t n, float *xq, size_t k, float *embeddings, bool *cacheHits);

    void evictCell(faiss::idx_t centroidIndex);


    ~Cache();
};


#endif