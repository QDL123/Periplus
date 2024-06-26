#ifndef CORE_H
#define CORE_H

#include "db_client.h"
#include "data.h"

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
    std::unique_ptr<faiss::IndexIDMap> idMap;
    std::shared_ptr<DBClient> db;    

    // size of nx * d 
    // std::vector< std::vector <float> > embeddings;

    std::vector<Data> data;

    Core(size_t d, std::shared_ptr<DBClient> db, size_t nCells, float nTotal);

    float getDensity(Data *x, size_t start, size_t range, faiss::idx_t target_centroid);
    // size_t getCellSize(Data *x, faiss::idx_t centroidIndex, size_t prevNGuess, size_t nGuess);

    // May not need this is we're training the 
    // whole dataset at once. This may just be a wrapper
    void train(faiss::idx_t n, const float* x);

    void loadCellWithVec(std::shared_ptr<float[]> xq);

    void evictCellWithVec(std::shared_ptr<float[]> xq);

    // void loadCell(faiss::idx_t centroidIndex);

    void loadCell(faiss::idx_t centroidIndex, float boundary_density);

    void search(size_t n, float *xq, size_t k, Data *data, int *cacheHits);

    void evictCell(faiss::idx_t centroidIndex);


    ~Core();
};


#endif