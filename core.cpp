#include "core.h"
#include "db_client.h"

#include <math.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>

#include <faiss/IndexIVFFlat.h>
#include <faiss/IndexFlat.h>
#include <faiss/IndexIDMap.h>


Core::Core(size_t d, std::shared_ptr<DBClient> db, size_t nCells, float nTotal) : d{d}, db{db}, nCells{nCells}, nTotal{nTotal}  {
    // TODO: use make_shared
    this->quantizer = std::shared_ptr<faiss::IndexFlatL2>(new faiss::IndexFlatL2(this->d));
    this->index = std::unique_ptr<faiss::IndexIVFFlat>(new faiss::IndexIVFFlat(this->quantizer.get(), this->d, this->nCells));
    this->idMap = std::unique_ptr<faiss::IndexIDMap>(new faiss::IndexIDMap(this->index.get()));
    this->residenceStatuses = std::unique_ptr<int[]>(new int32_t[(int32_t)this->nCells]);
    for (int i = 0; i < this->nCells; i++) {
        this->residenceStatuses[i] = -1;
    }
}


// Another layer will receive a stream of data, select a subset, and pass it here.
void Core::train(faiss::idx_t n, const float* x) {
    // Check this in case the training is done manually for testing purposes
    if (!this->index->is_trained) {
        this->index->train(n, x);
    }
    // Initialize an array of centroids that are stacked on each other
    // TODO: Is this the standard way of dealing with no knowing d at compile time?
    this->centroids = std::unique_ptr<float[]>(new float[this->d * this->nCells]);
    this->quantizer->reconstruct_n(0, this->nCells, this->centroids.get());
}

size_t Core::getCellSize(float *x, faiss::idx_t centroidIndex, size_t lowerBound, size_t upperBound) {
    size_t mid = (lowerBound + upperBound) / 2;
    faiss::idx_t midCentroid;
    float distance;
    this->quantizer->search(1, &x[mid * this->d], 1, &distance, &midCentroid);
    if (midCentroid != centroidIndex) {
        if (mid - 1 < lowerBound) {
            // mid - 1 is only < lowerBound if lowerBound == mid. The condition above guarantees there are no vectors
            // in this range belonging to the centroid of interest.
            // throw std::runtime_error(std::string("No vectors present belonging to centroidIndex"));
            return 0;
        }
        faiss::idx_t neighborCentroid;
        float neighborDistance;
        this->quantizer->search(1, &x[(mid - 1) * this->d], 1, &neighborDistance, &neighborCentroid);
        if (neighborCentroid == centroidIndex) {
            return mid;
        } else {
            // We're too far, bot the midCentroid and neighbor at -1 belong to a different cell
            return this->getCellSize(x, centroidIndex, lowerBound, mid + 1);
        }
    } else if (upperBound - lowerBound == 1) {
        // Range only spans 1 cell
        // TODO: Create a custom exception
        throw std::runtime_error(std::string("Data only spans a single cell"));
    } else {
        return this->getCellSize(x, centroidIndex, mid, upperBound);
    }
}

void Core::loadCell(faiss::idx_t centroidIndex) {
    // Ensure the index has been trained before we try to load any data
    assert(this->index->is_trained);
    // TODO: Look up the distribution to get a better guess here
    double ratio = double(1) / double(this->nCells);
    size_t prevNGuess = 0;
    size_t nGuess = size_t(nGuessCoeff * ratio * this->nTotal);
    float *x = new float[this->d * nGuess];
    faiss::idx_t furthestCentroid;
    float distances[1];
    // TODO: CHANGE THIS INTERFACE SO IT RETURNS THE SIZE OF THE RESULT IN CASE IT'S Smaller than nGuess;
    this->db->search(1, &this->centroids[centroidIndex * this->d], nGuess, x);
    faiss::idx_t centroid;
    float distance;

    float *furthestVec = &x[(nGuess - 1) * this->d];
    this->quantizer->search(1, furthestVec, 1, distances, &furthestCentroid);
    while (furthestCentroid == centroidIndex) {
        // Recompute nGuess
        prevNGuess = nGuess;
        nGuess = size_t(prevNGuess * guessScalar);
        // Reallocate x to fit new nGuess
        delete[] x;
        x = new float[this->d * nGuess];
        // Load data and determine furthest centroid
        this->db->search(1, &this->centroids[centroidIndex * this->d], nGuess, x);
        float *furthestVec = &x[(nGuess - 1) * this->d];
        this->quantizer->search(1, furthestVec, 1, distances, &furthestCentroid);
    }

    size_t cellSize = this->getCellSize(x, centroidIndex, prevNGuess, nGuess);
    
    // Need to append data into the data store.
    for (int i = 0; i < cellSize; ++i) {
        std::vector<float> embedding(x + (i * this->d), x + ((i + 1) * this->d));
        this->embeddings.push_back(embedding);
    }

    // Add the data to the index
    // this->index->add((faiss::idx_t)cellSize, x);
    faiss::idx_t ids[cellSize];
    for (faiss::idx_t i; i < cellSize; i++) {
        ids[i] = i;
    }
    this->idMap->add_with_ids((faiss::idx_t)cellSize, x, ids);
    this->residenceStatuses[centroidIndex] = (int32_t)cellSize;
    delete[] x;

    // TODO: update cell residency status
}

void Core::search(size_t n, float *xq, size_t k, float *embeddings, bool *cacheHits) {
    // Check residency status

    faiss::idx_t centroidIndices[n];
    float centroidDistances[n];

    // TODO: enable multi probe for more accurate queries (starting with nprobe = 1)
    size_t nprobe = 1;
    this->quantizer->search(n, xq, nprobe, centroidDistances, centroidIndices);
    for (int i = 0; i < n; i++) {
        if (this->residenceStatuses[centroidIndices[i]] > -1) {
            // cell is in residence
            cacheHits[i] = true;
            faiss::idx_t labels[k];
            float distances[k];
            this->index->search(1, &xq[i], k, distances, labels);
            for (int j = 0; j < k; j++) {
                // std::cout << "embedding value: " << this->embeddings[labels[j]] << std::endl;
                memcpy(&embeddings[(i * k * this->d) + (j * this->d)], this->embeddings[labels[j]].data(), sizeof(float) * this->d);
            }
        } else {
            cacheHits[i] = false;
            // Should we copy any data into the embeddigns array or leave it random data?
        }
    }
}

void Core::evictCell(faiss::idx_t centroidIndex) {
    if (this->residenceStatuses[centroidIndex] == -1) {
        throw std::runtime_error("Eviciting a cell not in residence");
    }

    // Look up what data needs to be removed
    // 2 methods of doing this: searching or directly looking up the invlist
    // faiss::idx_t indices[];
    // float distances[];
    // this->index->search(1, this->centroids[centroidIndex], this->residenceStatuses[centroidIndex], distances, indices);
    size_t cellSize = this->index->get_list_size(centroidIndex);
    std::vector<faiss::idx_t> indicesToRemove(cellSize);
    memcpy(indicesToRemove.data(), this->index->invlists->get_ids(centroidIndex), sizeof(faiss::idx_t) * cellSize);

    // Remove the data from the index
    faiss::IDSelectorBatch selector(cellSize, indicesToRemove.data());
    this->index->remove_ids(selector);

    // Remove the data
    std:sort(indicesToRemove.begin(), indicesToRemove.end(), std::greater<faiss::idx_t>());
    for (faiss::idx_t index : indicesToRemove) {
        this->embeddings.erase(embeddings.begin() + index);
    }

    // Update residency status
    this->residenceStatuses[centroidIndex] = -1;
}


Core::~Core() {
    // These deletes should not be necessary if smart pointers are being used.
    // delete this->quantizer;
    // delete this->index;
    // delete this->embeddings;
    // delete this->centroids;
}

