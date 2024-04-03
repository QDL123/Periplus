#include "cache.h"
#include "db_client.h"

#include <math.h>

#include <faiss/IndexIVFFlat.h>
#include <faiss/IndexFlat.h>


Cache::Cache(size_t d, DBClient *db) : d{d}, db{db} {
        // TODO: should we use smart pointers here to avoid memory leaks?
        this->quantizer = std::unique_ptr<faiss::Index>(new faiss::IndexFlatL2(this->d));
    }


// Another layer will receive a stream of data, select a subset, and pass it here.
void Cache::train(faiss::idx_t n, const float* x, float nTotal) {
    // compute the number of cells
    // This is based on guidance given by faiss for datasets under 1M vectors
    this->nTotal = nTotal;
    this->nCells = 16 * sqrt(nTotal);

    this->index = std::unique_ptr<faiss::Index>(new faiss::IndexIVFFlat(this->quantizer, this->d, this->nCells));
    this->index->train(n, x);
    // Initialize an array of centroids that are stacked on each other
    // TODO: Is this the standard way of dealing with no knowing d at compile time?
    this->centroids = new float[this->d * this->nCells];
    this->quantizer->reconstruct_n(0, this->nCells, this->centroids);
}

size_t Cache::getCellSize(float *x, faiss::idx_t centroidIndex, size_t lowerBound, size_t upperBound) {
    size_t mid = lowerBound + (upperBound - lowerBound) / 2;
    faiss::idx_t midCentroid;
    float distance;
    this->quantizer->search(1, x[mid * this->d], 1, &distance, &midCentroid);
    if (midCentroid != centroidIndex) {
        faiss::idx_t neighborCentroid;
        float neighborDistance;
        this->quantizer->search(1, x[(mid * this->d) - 1], 1, &neighborDistance, &neighborCentroid);

        if (neighborCentroid == centroidIndex) {
            return mid;
        } else {
            // We're too far, bot the midCentroid and neighbor at -1 belong to a different cell
            return this->getCellSize(x, centroidIndex, lowerBound, mid + 1);
        }
    } else {
        return this->getCellSize(x, centroidIndex, lowerBound, mid + 1);
    }
}

void Cache::loadCell(faiss::idx_t centroidIndex) {
    // TODO: Look up the distribution to get a better guess here
    double ratio = double(1 / this->nCells);
    size_t nGuess = size_t(nGuessCoeff * ratio * this->nTotal);
    float x[this->d * nGuess];

    faiss::idx_t labels = centroidIndex;
    float distances[1];
    size_t prevNGuess;
    while (labels == centroidIndex) {
        prevNGuess = nGuess;
        nGuess = size_t(prevNGuess * guessScalar);
        this->db->search(1, &this->centroids[centroidIndex], prevNGuess, x);
        float *furthestVec = x[(prevNGuess - 1) * this->d];
        this->quantizer->search(1, furthestVec, distances, &labels);
    }

    // x now contains prevNGuess vectors with d dimensions, and the furthest vector
    // is in another cell. Now we binary search to find the cell boundary
    size_t cellSize = this->getCellSize(x, centroidIndex, prevNGuess, nGuess);
    
    // Need to append data into the data store.
    for (int i = 0; i < cellSize; ++i) {
        std::vector<float> embedding(x + (i * this->d), x + ((i + 1) * ->d));
        this->embeddings.append(embedding);
    }

    // Add the data to the index
    this->index->add((faiss::idx_t)cellSize, x);

    // TODO: update cell residency status
}


Cache::~Cache() {
    // These deletes should not be necessary if smart pointers are being used.
    // delete this->quantizer;
    // delete this->index;
    delete this->data;
    delete this->centroids;
}

