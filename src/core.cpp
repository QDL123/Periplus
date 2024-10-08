#include "core.h"
#include "db_client.h"
#include "data.h"
#include "exceptions.h"

#include <math.h>
#include <memory>
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <fstream>

#include <faiss/IndexIVFFlat.h>
#include <faiss/IndexIVFPQ.h>
#include <faiss/IndexFlat.h>


Core::Core(size_t d, std::shared_ptr<DBClient> db, size_t nCells, float nTotal, bool use_flat) : d{d}, db{db}, nCells{nCells}, nTotal{nTotal}  {
    this->quantizer = std::shared_ptr<faiss::IndexFlatL2>(new faiss::IndexFlatL2(this->d));
    size_t m = 16; // TODO: adjust m to fit d (AutoTune?)

    // Low dimensional embeddings don't need to be product quantization
    // Embeddings with dimensions no divisible by m can't be quantized
    if (use_flat || (d < 64 || d % 8 != 0)) {
        std::cout << "instantiating IndexIVFFlat" << "\n";
        this->index = std::unique_ptr<faiss::IndexIVFFlat>(new faiss::IndexIVFFlat(this->quantizer.get(), this->d, this->nCells));
    } else {
        std::cout << "instantiating IndexIVFPQ" << "\n";
        this->index = std::unique_ptr<faiss::IndexIVFPQ>(new faiss::IndexIVFPQ(this->quantizer.get(), this->d, this->nCells, m, 8));
    }
    this->residence_statuses = std::unique_ptr<float[]>(new float[this->nCells]);
    for (size_t i = 0; i < this->nCells; i++) {
        this->residence_statuses[i] = -1;
        this->ids_by_cell.push_back(std::vector<std::string>());
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

void Core::loadCellWithVec(std::shared_ptr<float[]> xq, size_t nload) {
    faiss::idx_t centroidIndices[nload];
    float distances[nload];
    this->quantizer->search(1, xq.get(), nload, distances, centroidIndices);

    for (size_t i = 0; i < nload; i++) {
        if (this->residence_statuses[centroidIndices[i]] > -1) {
            std::cout << "Found cell already loaded: skipping\n";
            // throw std::runtime_error("Attempting to load cell already in residence. Must evict before loading again.");
            continue;
        }
        this->loadCell(centroidIndices[i]);
    }
}

void Core::evictCellWithVec(std::shared_ptr<float[]> xq, size_t nevict) {
    faiss::idx_t centroidIndices[nevict];
    float distances[nevict];
    this->quantizer->search(1, xq.get(), nevict, distances, centroidIndices);
    for (size_t i = 0; i < nevict; i++) {
        if (this->residence_statuses[centroidIndices[i]] < 0) {
            continue;
        }
        this->evictCell(centroidIndices[i]);
    }
}

// Load cell function based on id look up 
void Core::loadCell(faiss::idx_t target_centroid) {
    Data *x = new Data[this->ids_by_cell[target_centroid].size()];
    // TODO: return the size so if an id doesn't exist anymore it's okay
    try {
        this->db->search(this->ids_by_cell[target_centroid], x);
    } catch (const HttpException& e) {
        // deallocate x and rethrow
        delete[] x;
        throw e;
    }

    faiss::idx_t centroid;
    float distances[1];
    for (size_t i = 0; i < this->ids_by_cell[target_centroid].size(); i++) {
        assert(x[i].embedding.get() != nullptr);
        this->quantizer->search(1, x[i].embedding.get(), 1, distances, &centroid);
        if (centroid == target_centroid) {
            assert(this->id_map.find(std::string(x[i].id.get())) != this->id_map.end());
            faiss::idx_t id_num = this->id_map[std::string(x[i].id.get())];

            // Assert that this data doesn't already exist in the data map
            assert(this->data_map.find(id_num) == this->data_map.end());
            this->data_map.insert({id_num, x[i]});

            // TODO: look into doing this as a group operation instead of 1 at a time
            this->index->add_with_ids(1, x[i].embedding.get(), &id_num);
        } else {
            std::cerr << "Queried vector does not belong to the target cell" << std::endl;
            std::cerr << "Expected " << target_centroid << ", but quantizer search returned " << centroid << std::endl;
        }
    }

    this->residence_statuses[target_centroid] = this->ids_by_cell[target_centroid].size();
    delete[] x;
}

// TODO: return distances also
void Core::search(size_t n, float *xq, size_t k, size_t nprobe, bool require_all, Data *data, int *cacheHits) {
    // Check residency status
    this->index->nprobe = nprobe;
    faiss::idx_t centroidIndices[n * nprobe];
    float centroidDistances[n * nprobe];

    this->quantizer->search(n, xq, nprobe, centroidDistances, centroidIndices);
    for (int i = 0; i < n; i++) {
        bool cacheHit = this->residence_statuses[centroidIndices[i * nprobe]] > -1;
        if (require_all) {
            for (int j = 1; cacheHit && j < nprobe; j++) {
                if (this->residence_statuses[centroidIndices[(i * nprobe) + j]] == -1) {
                    // Found nearby centroid not in residence
                    cacheHit = false;
                }
            }
        }
        if (cacheHit) {
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
                    // If this assertion is violtated, it means there is is inconsistency between the index and data map
                    assert(this->data_map.find(labels[j]) != this->data_map.end());
                    data[(i * k) + j] = this->data_map[labels[j]];
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
    if (this->residence_statuses[centroidIndex] == -1) {
        throw std::runtime_error("Eviciting a cell not in residence");
    }

    size_t cellSize = this->index->get_list_size(centroidIndex);
    std::vector<faiss::idx_t> idsToRemove(cellSize);
    // Copy ids that need to be removed from the data map
    memcpy(idsToRemove.data(), this->index->invlists->get_ids(centroidIndex), sizeof(faiss::idx_t) * cellSize);

    // Remove data from the index by deleting relevant invlist
    this->index->invlists->resize(centroidIndex, 0);

    // Remove data from the data map
    for (auto itr = idsToRemove.begin(); itr != idsToRemove.end(); itr++) {
        this->data_map.erase(*itr);
    }

    // Update cell residency status
    this->residence_statuses[centroidIndex] = -1;
}

// TODO: Remove this check
bool Core::isNullTerminated(const char* str, size_t max_length) {
    for (size_t i = 0; i < max_length; ++i) {
        if (str[i] == '\0') {
            return true;  // Found null terminator within the maxLength
        }
    }
    return false;  // No null terminator found within the maxLength
}


void Core::add(size_t num_docs, std::vector<std::shared_ptr<char[]>>& ids, std::shared_ptr<float[]> embeddings) {
    // Ensure the number of embeddings matches the number of ids
    faiss::idx_t *updated_centroids = new faiss::idx_t[num_docs];
    float *distances = new float[num_docs];
    this->quantizer->search(num_docs, embeddings.get(), 1, distances, updated_centroids);
    delete[] distances;
    for (size_t i = 0; i < num_docs; i++) {
        assert(this->isNullTerminated(ids[i].get(), 100));
        std::string id(ids[i].get());
        this->ids_by_cell[updated_centroids[i]].push_back(id);
        this->id_map.insert({std::string(ids[i].get()), this->next_id});
        this->next_id++;
    }
    delete[] updated_centroids;
}


Core::~Core() {}
