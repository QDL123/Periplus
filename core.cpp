#include "core.h"
#include "db_client.h"
#include "data.h"

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
    this->residenceStatuses = std::unique_ptr<float[]>(new float[this->nCells]);
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

void Core::loadCellWithVec(std::shared_ptr<float[]> xq) {
    faiss::idx_t centroidIndex;
    float distance;
    this->quantizer->search(1, xq.get(), 1, &distance, &centroidIndex);
    if (this->residenceStatuses[centroidIndex] > -1) {
        // The cell has already been loaded. Must be evicted before it is loaded again
        // TODO: Make this a customer error and catch it in the calling method to send the message back to
        // the client
        throw std::runtime_error("Attempting to load cell already in residence. Must evict before loading again.");
    }
    std::cout << "Calling core loadCell func" << std::endl;
    // TODO: Decide how to set this hyperparameter
    float density = 0.1;
    this->loadCell(centroidIndex, density);
    std::cout << "Completed Cell Load" << std::endl;
}

void Core::evictCellWithVec(std::shared_ptr<float[]> xq) {
    faiss::idx_t centroidIndex;
    float distance;
    this->quantizer->search(1, xq.get(), 1, &distance, &centroidIndex);
    if (this->residenceStatuses[centroidIndex] < 0) {
        throw std::runtime_error("Attempting to evict cell not in residence.");
    }
    std::cout << "Calling core evictCell func" << std::endl;
    this->evictCell(centroidIndex);
    std::cout << "Completed Cell Eviction" << std::endl;
}

// size_t Core::getCellSize(Data *x, faiss::idx_t centroidIndex, size_t lowerBound, size_t upperBound) {
//     size_t mid = (lowerBound + upperBound) / 2;
//     faiss::idx_t midCentroid;
//     float distance;
//     // this->quantizer->search(1, &x[mid * this->d], 1, &distance, &midCentroid);
//     this->quantizer->search(1, x[mid].embedding.get(), 1, &distance, &midCentroid);
//     if (midCentroid != centroidIndex) {
//         if (mid - 1 < lowerBound) {
//             // mid - 1 is only < lowerBound if lowerBound == mid. The condition above guarantees there are no vectors
//             // in this range belonging to the centroid of interest.
//             // throw std::runtime_error(std::string("No vectors present belonging to centroidIndex"));
//             return 0;
//         }
//         faiss::idx_t neighborCentroid;
//         float neighborDistance;
//         // TODO: Investigate if we can only receive embeddings while trying to determine cellSize
//         this->quantizer->search(1, x[mid - 1].embedding.get(), 1, &neighborDistance, &neighborCentroid);
//         if (neighborCentroid == centroidIndex) {
//             return mid;
//         } else {
//             // We're too far, bot the midCentroid and neighbor at -1 belong to a different cell
//             return this->getCellSize(x, centroidIndex, lowerBound, mid + 1);
//         }
//     } else if (upperBound - lowerBound == 1) {
//         // Range only spans 1 cell
//         // TODO: Create a custom exception
//         throw std::runtime_error(std::string("Data only spans a single cell"));
//     } else {
//         return this->getCellSize(x, centroidIndex, mid, upperBound);
//     }
// }

float Core::getDensity(Data *x, size_t start, size_t range, faiss::idx_t target_centroid) {
    size_t numInCell = 0;
    for (size_t i = start; i < start + range; i++) {
        float distances[1];
        faiss::idx_t centroid;
        this->quantizer->search(1, x[i].embedding.get(), 1, distances, &centroid);
        if (centroid == target_centroid) {
            numInCell++;
        }
    }

    float density = (float)numInCell / range;
    std::cout << "Density of centroid: " << target_centroid << " from " << start << " to " << start + range << " is: " << density << std::endl;  
    return density;
}

void Core::loadCell(faiss::idx_t target_centroid, float boundary_density) {
    assert(this->index->is_trained);
    double ratio = double(1) / double(this->nCells);
    size_t prevNGuess = 0;
    size_t nGuess = size_t(nGuessCoeff * ratio * this->nTotal);
    // float *x = new float[this->d * nGuess];
    Data *x = new Data[nGuess];
    faiss::idx_t furthestCentroid;
    float distances[1];
    // TODO: CHANGE THIS INTERFACE SO IT RETURNS THE SIZE OF THE RESULT IN CASE IT'S Smaller than nGuess;
    this->db->search(1, &this->centroids[target_centroid * this->d], nGuess, x);

    // Check density of final 50% of nGuess
    size_t range = (double)nGuess * (1);
    float density = this->getDensity(x, nGuess - range, range, target_centroid);

    while (density > boundary_density) {
        // Recompute nGuess
        prevNGuess = nGuess;
        nGuess = size_t(prevNGuess * guessScalar);
        // Reallocate x to fit new nGuess
        delete[] x;
        // x = new float[this->d * nGuess];
        x = new Data[nGuess];
        // Load data and determine furthest centroid
        this->db->search(1, &this->centroids[target_centroid * this->d], nGuess, x);

        size_t range = (double)nGuess * (1);
        density = this->getDensity(x, nGuess - range, range, target_centroid); 
    }

    std::cout << "Fetched enough data. Size: " << nGuess << std::endl;

    float longestDistance = 0;
    faiss::idx_t centroid;
    size_t cellSize = 0;
    for (size_t i = 0; i < nGuess; i++) {
        this->quantizer->search(1, x[i].embedding.get(), 1, distances, &centroid);
        if (centroid == target_centroid) {
            this->data.push_back(x[i]);
            this->index->add(1, x[i].embedding.get());
            cellSize++;
            // This should always be true since we are going in order
            assert(distances[0] >= longestDistance);
            longestDistance = distances[0];
        }
    }

    std::cout << "Loaded "  << cellSize << " data points into cell" << std::endl; 

    // TODO: How are we tracking the furthest distance?
    this->residenceStatuses[target_centroid] = longestDistance;
    delete[] x;
    std::cout << "Finished clean up" << std::endl;
}

// void Core::loadCell(faiss::idx_t centroidIndex) {
//     // Ensure the index has been trained before we try to load any data
//     assert(this->index->is_trained);
//     // TODO: Look up the distribution to get a better guess here
//     double ratio = double(1) / double(this->nCells);
//     size_t prevNGuess = 0;
//     size_t nGuess = size_t(nGuessCoeff * ratio * this->nTotal);
//     // float *x = new float[this->d * nGuess];
//     Data *x = new Data[nGuess];
//     faiss::idx_t furthestCentroid;
//     float distances[1];
//     // TODO: CHANGE THIS INTERFACE SO IT RETURNS THE SIZE OF THE RESULT IN CASE IT'S Smaller than nGuess;
//     this->db->search(1, &this->centroids[centroidIndex * this->d], nGuess, x);
//     faiss::idx_t centroid;
//     float distance;

//     // float *furthestVec = &x[(nGuess - 1) * this->d];
//     float *furthestVec = x[nGuess - 1].embedding.get();
//     this->quantizer->search(1, furthestVec, 1, distances, &furthestCentroid);
//     while (furthestCentroid == centroidIndex) {
//         // Recompute nGuess
//         prevNGuess = nGuess;
//         nGuess = size_t(prevNGuess * guessScalar);
//         // Reallocate x to fit new nGuess
//         delete[] x;
//         // x = new float[this->d * nGuess];
//         x = new Data[nGuess];
//         // Load data and determine furthest centroid
//         this->db->search(1, &this->centroids[centroidIndex * this->d], nGuess, x);
//         // float *furthestVec = &x[(nGuess - 1) * this->d];
//         float *furthestVec = x[nGuess - 1].embedding.get();
//         this->quantizer->search(1, furthestVec, 1, distances, &furthestCentroid);
//     }

//     size_t cellSize = this->getCellSize(x, centroidIndex, prevNGuess, nGuess);
//     std::cout << "Cell Size: " << cellSize << std::endl;
//     // Need to append data into the data store.
//     for (int i = 0; i < cellSize; ++i) {
//         this->data.push_back(x[i]);
//     }

//     // Add the data to the index
//     // Looks like we were trying to do this another way to take advantage of the fact 
//     // that we are adding to a single invlist. We can look into optimizing this later.
//     // this->index->add((faiss::idx_t)cellSize, x);
//     // faiss::idx_t ids[cellSize];
//     // for (faiss::idx_t i; i < cellSize; i++) {
//     //     ids[i] = i;
//     // }
//     // this->idMap->add_with_ids((faiss::idx_t)cellSize, x, ids);
//     for (size_t i = 0; i < cellSize; i++) {
//         this->index->add(1, x[i].embedding.get());
//     }

//     // WARNING: THIS NEEDS TO BE UPDATED FOR THIS FUNCTION TO WORK
//     this->residenceStatuses[centroidIndex] = (int32_t)cellSize;
//     delete[] x;

//     for (size_t i = 0; i < this->nCells; i++) {
//         size_t cellSize = this->index->get_list_size(i);
//         if (cellSize > 0) {
//             std::cout << "Found cell with size: " << cellSize << "\n";
//         }
//     }

//     std::cout << "data vector size: " << this->data.size() << std::endl;
// }

// TODO: return distances also
void Core::search(size_t n, float *xq, size_t k, Data *data, int *cacheHits) {
    // Check residency status

    faiss::idx_t centroidIndices[n];
    float centroidDistances[n];

    // TODO: enable multi probe for more accurate queries (starting with nprobe = 1)
    size_t nprobe = 1;
    std::cout << "query[0]: " << xq[0] << ", query[499]: " << xq[499];
    this->quantizer->search(n, xq, nprobe, centroidDistances, centroidIndices);
    std::cout << ", centroid[0]: " << this->centroids[centroidIndices[0] * this->d + 0] << ", centroid[499]: " << this->centroids[centroidIndices[0] * this->d + 499] << std::endl;
    for (int i = 0; i < n; i++) {
        // TODO: experiement with this condition. Consider using median based (excluding queries further than the 90th
        // percentile vector) rather than 90% of the distance (this much harder)
        float status = this->residenceStatuses[centroidIndices[i]];
        std::cout << "furthest vector loaded in cell: " << status << std::endl;
        std::cout << "distance of query vector: " << centroidDistances[i] << std::endl;
        if (status > -1 && centroidDistances[i] < status) {
            // cell is in residence
            cacheHits[i] = 0;
            faiss::idx_t labels[k];
            float distances[k];
            this->index->search(1, &xq[i], k, distances, labels);
            for (int j = 0; j < k; j++) {
                if (labels[j] == -1) {
                    // Fewer than k results, padded with -1
                    data[(i * k) + j] = Data();
                } else {
                    // Use copy constructor
                    data[(i * k) + j] = this->data[labels[j]];
                    cacheHits[i]++;
                }
            }
        } else {
            cacheHits[i] = -1;
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
    std::cout << "Evicting cell size: " << cellSize << std::endl;
    std::vector<faiss::idx_t> indicesToRemove(cellSize);
    memcpy(indicesToRemove.data(), this->index->invlists->get_ids(centroidIndex), sizeof(faiss::idx_t) * cellSize);

    // Remove the data from the index
    faiss::IDSelectorBatch selector(cellSize, indicesToRemove.data());
    this->index->remove_ids(selector);

    // Remove the data
    std:sort(indicesToRemove.begin(), indicesToRemove.end(), std::greater<faiss::idx_t>());
    for (faiss::idx_t index : indicesToRemove) {
        // this->embeddings.erase(embeddings.begin() + index);
        this->data.erase(data.begin() + index);
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

