#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "core.h"
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <faiss/IndexIVFFlat.h>
#include <faiss/IndexFlat.h>


TEST_CASE("Construct Cache Core", "[Core]") {
    size_t d = 10;
    float nTotal = 10000;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient(d));
    Core core(d, client, 100, nTotal);

    REQUIRE(core.d == d);
}

TEST_CASE("Train the cache core", "[Core::train]") {
    // Create the cache core
    size_t d = 10;
    float nTotal = 10000;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient(d));
    Core core(d, client, 4, nTotal);

    // Build the dataset
    faiss::idx_t n = 100000;
    float x[n * d];
    for (int i = 0; i < n * d; i++) {
        x[i] = float(i);
    }
    // Train the cache core
    core.train(n, x);

    // Assert
    // ensure the centroid pointer is written to
    REQUIRE(core.centroids.get() != nullptr);
    // ensure the index pointer is written to
    REQUIRE(core.index.get() != nullptr);

    // Figure out what else we can assert here...
}

TEST_CASE("Get Cell Size", "[Core::getCellSize]") {
    // Create cache core
    size_t d = 2;
    float nTotal = 10000;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient(d));
    Core core(d, client, 4, nTotal);
    assert(core.index->is_trained == false);

    // Manually set the centroids for testing purposes
    const float centroids[] = {1000, 1000, -1000, 1000, 1000, -1000, -1000, -1000};
    core.quantizer->add(4, centroids);

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
    core.index->is_trained = true;
    assert(core.index->is_trained);
    core.train(800, data);

    // Test Spanning 3 cells
    size_t cellSize = core.getCellSize(data, 0, 0, 250);
    REQUIRE(cellSize == 100);

    // Test Spanning 2 cells
    cellSize = core.getCellSize(data, 0, 0, 110);
    REQUIRE(cellSize == 100);

    // Test Spanning 2 cells by 1
    cellSize = core.getCellSize(data, 0, 0, 101);
    REQUIRE(cellSize == 100);

    // Test tighest window
    // With current function formulation upperbound will never be considered for the first outside 
    // target cell so the function is upper bound exclusive. It can safely be out of bounds or the size of the array
    // This makes sense considering the array will be size nGuess but the last element in the array will be at index nGuess - 1.
    cellSize = core.getCellSize(data, 0, 99, 101);
    REQUIRE(cellSize == 100);

    // Test spanning only 1 cell (should throw exception)
    REQUIRE_THROWS(core.getCellSize(data, 0, 0, 100));
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

TEST_CASE("Load Cell", "[Core::loadCell]") {
    // Create cache core
    size_t d = 2;
    float nTotal = 800;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient(d));
    size_t nCells = 4;
    Core core(d, client, nCells, nTotal);

    // Manually set the centroids for testing purposes
    const float centroids[] = {100, 100, -100, 100, 100, -100, -100, -100};
    core.quantizer->add(nCells, centroids);

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
    core.index->is_trained = true;
    core.train(n, data);

    // Load the external db the cache core pulls from
    client->loadDB(n, data);

    // Load the first cell
    core.loadCell(0);
    REQUIRE(core.embeddings.size() == 100);

    // Load the 4th cell
    core.loadCell(3);
    REQUIRE(core.embeddings.size() == 200);


    // Test multiple db queries needed to get full cell (Should need to make 3 total)
    core.nTotal = 100;
    core.loadCell(1);
    REQUIRE(core.embeddings.size() == 300);
    // Is there a way to assert how many times db->search is being called to validate what we're testing?
}


TEST_CASE("Search", "[Core::search]") {
    // Create cache core
    size_t d = 2;
    float nTotal = 800;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient(d));
    size_t nCells = 4;
    Core core(d, client, nCells, nTotal);

    // Manually set the centroids for testing purposes
    const float centroids[] = {100, 100, -100, 100, 100, -100, -100, -100};
    core.quantizer->add(nCells, centroids);

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
    core.index->is_trained = true;
    core.train(n, data);

    // Load the external db the cache core pulls from
    client->loadDB(n, data);

    // Load the first cell
    core.loadCell(0);

    size_t xq_n = 1;
    size_t k = 5;
    float embeddings[k * d];
    bool cacheHits[n];
    float xq[xq_n];
    xq[0] = centroids[0];
    xq[1] = centroids[1];


    core.search(xq_n, xq, k, embeddings, cacheHits);
    REQUIRE(cacheHits[0] == true);
    

    // Make sure all the embeddings come from the first cell
    for (int i = 0; i < xq_n * k * d; i++) {
        REQUIRE((embeddings[i] <= 105 && embeddings[i] >= 95));
    }

    xq[0] = centroids[6];
    xq[1] = centroids[7];
    core.search(xq_n, xq, k, embeddings, cacheHits);
    // Length is n so there should only be 1
    REQUIRE(cacheHits[0] == false);

    core.loadCell(3);
    core.search(xq_n, xq, k, embeddings, cacheHits);
    REQUIRE(cacheHits[0] == true);
    for (int i = 0; i < xq_n * k * d; i++) {
        REQUIRE((embeddings[i] <= -95 && embeddings[i] >= -105));
    }
}


TEST_CASE("Evict cell", "[Core::evictCell]") {
    // Create Cache Core
    size_t d = 2;
    float nTotal = 800;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient(d));
    size_t nCells = 4;
    Core core(d, client, nCells, nTotal);

    // Manually set the centroids for testing purposes
    const float centroids[] = {100, 100, -100, 100, 100, -100, -100, -100};
    core.quantizer->add(nCells, centroids);

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
    core.index->is_trained = true;
    core.train(n, data);

    // Load the external db the cache core pulls from
    client->loadDB(n, data);

    // Load the first cell
    core.loadCell(0);

    // Remove the cell
    core.evictCell(0);
    REQUIRE(core.embeddings.size() == 0);
    
    core.loadCell(1);
    core.loadCell(3);
    REQUIRE(core.embeddings.size() == 200);

    size_t xq_n = 1;
    size_t k = 5;
    float embeddings[k * d];
    bool cacheHits[n];
    float xq[xq_n];
    xq[0] = centroids[6];
    xq[1] = centroids[7];

    core.search(xq_n, xq, k, embeddings, cacheHits);
    REQUIRE(cacheHits[0] == true);
    for (int i = 0; i < xq_n * k * d; i++) {
        REQUIRE((embeddings[i] <= -95 && embeddings[i] >= -105));
    }

    core.evictCell(3);
    core.search(xq_n, xq, k, embeddings, cacheHits);
    REQUIRE(cacheHits[0] == false);
    REQUIRE(core.embeddings.size() == 100);
    
    core.evictCell(1);
    REQUIRE(core.embeddings.size() == 0);
}
