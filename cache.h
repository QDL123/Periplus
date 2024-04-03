#ifndef CACHE_H
#define CACHE_H

#include <faiss/Index.h>
#include <DBClient.h>

struct Cache {

    static double nGuessCoeff = 2;
    static double guessScalar = 1.5;

    float nTotal = 0;
    size_t d = 0;
    size_t nCells = 0;
    float* centroids = nullptr;
    std::unique_ptr<faiss::Index> quantizer;
    std::unique_ptr<faiss::Index> index;
    std::shared_ptr<DBClient> db;    

    // size of nx * d 
    std::vector< std::vector <float> > embeddings;

    Cache(size_t d, DBClient *db);

    size_t getCellSize(float *x, faiss::idx_t centroidIndex, size_t prevNGuess, size_t nGuess);

    // May not need this is we're training the 
    // whole dataset at once. This may just be a wrapper
    void train(faiss::idx_t n, const float* x, float nTotal);

    void loadCell(faiss::idx_t centroidIndex);


    ~Cache();
};


#endif