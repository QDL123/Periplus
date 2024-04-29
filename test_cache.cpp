#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "cache.h"
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <faiss/IndexIVFFlat.h>
#include <faiss/IndexFlat.h>


TEST_CASE("Construct Cache", "[Cache]") {
    size_t d = 10;
    float nTotal = 10000;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient(d));
    Cache cache(d, client, 100, nTotal);

    REQUIRE(cache.d == d);
}

TEST_CASE("Train the cache", "[Cache::train]") {
    // Create the cache
    size_t d = 10;
    float nTotal = 10000;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient(d));
    Cache cache(d, client, 4, nTotal);

    // Build the dataset
    faiss::idx_t n = 100000;
    float x[n * d];
    for (int i = 0; i < n * d; i++) {
        x[i] = float(i);
    }
    // Train the cache
    cache.train(n, x);

    // Assert
    // ensure the centroid pointer is written to
    REQUIRE(cache.centroids.get() != nullptr);
    // ensure the index pointer is written to
    REQUIRE(cache.index.get() != nullptr);

    // Figure out what else we can assert here...
}

TEST_CASE("Get Cell Size", "[Cache::getCellSize]") {
    // Create cache
    size_t d = 2;
    float nTotal = 10000;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient(d));
    Cache cache(d, client, 4, nTotal);
    assert(cache.index->is_trained == false);

    // Manually set the centroids for testing purposes
    const float centroids[] = {1000, 1000, -1000, 1000, 1000, -1000, -1000, -1000};
    cache.quantizer->add(4, centroids);

    // Generate data
    faiss::idx_t n = 800;
    float data[n];
    for (int i = 0; i < 8; i += 2) {
        for (int j = 0; j < 100; j++) {
            float x = (rand() % 50) - 25;
            float y = (rand() % 50) - 25;
            data[i * 100 + j * 2] = centroids[i] + x;
            data[i * 100 + j * 2 + 1] = centroids[i + 1] + y;
        }
    }

    // Set the index to trained so the centroids are not modified
    cache.index->is_trained = true;
    assert(cache.index->is_trained);
    cache.train(800, data);

    // Test Spanning 3 cells
    size_t cellSize = cache.getCellSize(data, 0, 0, 250);
    REQUIRE(cellSize == 100);

    // Test Spanning 2 cells
    cellSize = cache.getCellSize(data, 0, 0, 110);
    REQUIRE(cellSize == 100);

    // Test Spanning 2 cells by 1
    cellSize = cache.getCellSize(data, 0, 0, 101);
    REQUIRE(cellSize == 100);

    // Test tighest window
    // With current function formulation upperbound will never be considered for the first outside 
    // target cell so the function is upper bound exclusive. It can safely be out of bounds or the size of the array
    // This makes sense considering the array will be size nGuess but the last element in the array will be at index nGuess - 1.
    cellSize = cache.getCellSize(data, 0, 99, 101);
    REQUIRE(cellSize == 100);

    // Test spanning only 1 cell (should throw exception)
    REQUIRE_THROWS(cache.getCellSize(data, 0, 0, 100));
}


// Move this to a separate test file later, but it will be totally different then anyway.
TEST_CASE("DB Client", "[DBClient::loadDB]") {
    size_t d = 2;
    std::unique_ptr<DBClient> client = std::unique_ptr<DBClient>(new DBClient(d));

    // Generate data
    const float centroids[] = {1000, 1000, -1000, 1000, 1000, -1000, -1000, -1000};
    faiss::idx_t n = 800;
    float data[n];
    for (int i = 0; i < 8; i += 2) {
        for (int j = 0; j < 100; j++) {
            float x = (rand() % 50) - 25;
            float y = (rand() % 50) - 25;
            data[i * 100 + j * 2] = centroids[i] + x;
            data[i * 100 + j * 2 + 1] = centroids[i + 1] + y;
        }
    }

    // Load the database
    client->loadDB(n, data);

    // Query the full database
    float results[n];
    client->search(1, data, n, results);

    // TODO: Figure out what we can assert here...
}

TEST_CASE("Load Cell", "[Cache::loadCell]") {
    // Create Cache
    size_t d = 2;
    float nTotal = 800;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient(d));
    size_t nCells = 4;
    Cache cache(d, client, nCells, nTotal);

    // Manually set the centroids for testing purposes
    const float centroids[] = {100, 100, -100, 100, 100, -100, -100, -100};
    cache.quantizer->add(nCells, centroids);

    // Generate dataset
    // TODO: Make this a separate funtion
    std::vector<std::pair<int, int>> centroidPairs = {{100, 100}, {-100, 100}, {100, -100}, {-100, -100}};
    faiss::idx_t numPerCentroid = 100;
    faiss::idx_t n = numPerCentroid * nCells;
    float data[n * d];
    for (int i = 0; i < centroidPairs.size(); i++) {
        for (int j = 0; j < numPerCentroid; j++) {
            float x = (rand() % 10) - 5;
            float y = (rand() % 10) - 5;
            data[(i * numPerCentroid + j) * d] = centroidPairs[i].first + x;
            data[((i * numPerCentroid + j) * d) + 1] = centroidPairs[i].second + y;
        }
    }

    // Train the index
    cache.index->is_trained = true;
    cache.train(n, data);

    // Load the external db the cache pulls from
    client->loadDB(n, data);

    // Load the first cell
    cache.loadCell(0);
    REQUIRE(cache.embeddings.size() == 100);

    // Load the 4th cell
    cache.loadCell(3);
    REQUIRE(cache.embeddings.size() == 200);


    // Test multiple db queries needed to get full cell (Should need to make 3 total)
    cache.nTotal = 100;
    cache.loadCell(1);
    REQUIRE(cache.embeddings.size() == 300);
    // Is there a way to assert how many times db->search is being called to validate what we're testing?
}


TEST_CASE("Search", "[Cache::search]") {
    // Create Cache
    size_t d = 2;
    float nTotal = 800;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient(d));
    size_t nCells = 4;
    Cache cache(d, client, nCells, nTotal);

    // Manually set the centroids for testing purposes
    const float centroids[] = {100, 100, -100, 100, 100, -100, -100, -100};
    cache.quantizer->add(nCells, centroids);

    // Generate dataset
    // TODO: Make this a separate funtion
    std::vector<std::pair<int, int>> centroidPairs = {{100, 100}, {-100, 100}, {100, -100}, {-100, -100}};
    faiss::idx_t numPerCentroid = 100;
    faiss::idx_t n = numPerCentroid * nCells;
    float data[n * d];
    for (int i = 0; i < centroidPairs.size(); i++) {
        for (int j = 0; j < numPerCentroid; j++) {
            float x = (rand() % 10) - 5;
            float y = (rand() % 10) - 5;
            data[(i * numPerCentroid + j) * d] = centroidPairs[i].first + x;
            data[((i * numPerCentroid + j) * d) + 1] = centroidPairs[i].second + y;
        }
    }

    // Train the index
    cache.index->is_trained = true;
    cache.train(n, data);

    // Load the external db the cache pulls from
    client->loadDB(n, data);

    // Load the first cell
    cache.loadCell(0);

    size_t xq_n = 1;
    size_t k = 5;
    float embeddings[k * d];
    bool cacheHits[n];
    float xq[xq_n];
    xq[0] = centroids[0];
    xq[1] = centroids[1];


    cache.search(xq_n, xq, k, embeddings, cacheHits);
    REQUIRE(cacheHits[0] == true);
    

    // Make sure all the embeddings come from the first cell
    for (int i = 0; i < xq_n * k * d; i++) {
        REQUIRE((embeddings[i] <= 105 && embeddings[i] >= 95));
    }

    xq[0] = centroids[6];
    xq[1] = centroids[7];
    cache.search(xq_n, xq, k, embeddings, cacheHits);
    // Length is n so there should only be 1
    REQUIRE(cacheHits[0] == false);

    cache.loadCell(3);
    cache.search(xq_n, xq, k, embeddings, cacheHits);
    REQUIRE(cacheHits[0] == true);
    for (int i = 0; i < xq_n * k * d; i++) {
        REQUIRE((embeddings[i] <= -95 && embeddings[i] >= -105));
    }
}


TEST_CASE("Evict cell", "[Cache::evictCell]") {
    // Create Cache
    size_t d = 2;
    float nTotal = 800;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient(d));
    size_t nCells = 4;
    Cache cache(d, client, nCells, nTotal);

    // Manually set the centroids for testing purposes
    const float centroids[] = {100, 100, -100, 100, 100, -100, -100, -100};
    cache.quantizer->add(nCells, centroids);

    // Generate dataset
    // TODO: Make this a separate funtion
    std::vector<std::pair<int, int>> centroidPairs = {{100, 100}, {-100, 100}, {100, -100}, {-100, -100}};
    faiss::idx_t numPerCentroid = 100;
    faiss::idx_t n = numPerCentroid * nCells;
    float data[n * d];
    for (int i = 0; i < centroidPairs.size(); i++) {
        for (int j = 0; j < numPerCentroid; j++) {
            float x = (rand() % 10) - 5;
            float y = (rand() % 10) - 5;
            data[(i * numPerCentroid + j) * d] = centroidPairs[i].first + x;
            data[((i * numPerCentroid + j) * d) + 1] = centroidPairs[i].second + y;
        }
    }

    // Train the index
    cache.index->is_trained = true;
    cache.train(n, data);

    // Load the external db the cache pulls from
    client->loadDB(n, data);

    // Load the first cell
    cache.loadCell(0);

    // Remove the cell
    cache.evictCell(0);
    REQUIRE(cache.embeddings.size() == 0);
    
    cache.loadCell(1);
    cache.loadCell(3);
    REQUIRE(cache.embeddings.size() == 200);

    size_t xq_n = 1;
    size_t k = 5;
    float embeddings[k * d];
    bool cacheHits[n];
    float xq[xq_n];
    xq[0] = centroids[6];
    xq[1] = centroids[7];

    cache.search(xq_n, xq, k, embeddings, cacheHits);
    REQUIRE(cacheHits[0] == true);
    for (int i = 0; i < xq_n * k * d; i++) {
        REQUIRE((embeddings[i] <= -95 && embeddings[i] >= -105));
    }

    cache.evictCell(3);
    cache.search(xq_n, xq, k, embeddings, cacheHits);
    REQUIRE(cacheHits[0] == false);
    REQUIRE(cache.embeddings.size() == 100);
    
    cache.evictCell(1);
    REQUIRE(cache.embeddings.size() == 0);
}
