#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "data.h"
#include "core.h"
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <faiss/IndexIVFFlat.h>
#include <faiss/IndexFlat.h>


void generate_data(size_t d, const float *centroids, std::vector<Data>& data, std::vector<float>& embeddings) {
    // Generate data
    for (int i = 0; i < 8; i += 2) {
        for (int j = 0; j < 100; j++) {

            float x = (rand() % 10) - 5;
            float y = (rand() % 10) - 5;

            embeddings.push_back(centroids[i] + x);
            embeddings.push_back(centroids[i + 1] + y);
        }
    }

    // Generate data structs
    for (int i = 0; i < 8; i += 2) {
        for (int j = 0; j < 100; j++) {
            char id[2];
            id[0] = 'i';
            id[1] = 'd';
            
            float embedding[d];
            
            embedding[0] = embeddings[i * 100 + j * 2];
            embedding[1] = embeddings[i * 100 + j * 2 + 1];        

            char document[3];
            document[0] = 'd';
            document[1] = 'o';
            document[2] = 'c';

            char metadata[4];
            metadata[0] = 'm';
            metadata[1] = 'e';
            metadata[2] = 't';
            metadata[3] = 'a';

            data.push_back(Data(2, d, 3, 4, id, embedding, document, metadata));
        }
    }
}


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

    std::vector<Data> data;
    std::vector<float> embeddings;
    faiss::idx_t n = 800;
    generate_data(d, centroids, data, embeddings);

    // Set the index to trained so the centroids are not modified
    core.index->is_trained = true;
    assert(core.index->is_trained);
    core.train(800, embeddings.data());

    // Test Spanning 3 cells
    size_t cellSize = core.getCellSize(data.data(), 0, 0, 250);
    REQUIRE(cellSize == 100);

    // Test Spanning 2 cellss
    cellSize = core.getCellSize(data.data(), 0, 0, 110);
    REQUIRE(cellSize == 100);

    // Test Spanning 2 cells by 1
    cellSize = core.getCellSize(data.data(), 0, 0, 101);
    REQUIRE(cellSize == 100);

    // Test tighest window
    // With current function formulation upperbound will never be considered for the first outside 
    // target cell so the function is upper bound exclusive. It can safely be out of bounds or the size of the array
    // This makes sense considering the array will be size nGuess but the last element in the array will be at index nGuess - 1.
    cellSize = core.getCellSize(data.data(), 0, 99, 101);
    REQUIRE(cellSize == 100);

    // Test spanning only 1 cell (should throw exception)
    REQUIRE_THROWS(core.getCellSize(data.data(), 0, 0, 100));
}


// Move this to a separate test file later, but it will be totally different then anyway.
TEST_CASE("DB Client", "[DBClient::loadDB]") {
    size_t d = 2;
    std::unique_ptr<DBClient> client = std::unique_ptr<DBClient>(new DBClient(d));

    // Generate data
    const float centroids[] = {1000, 1000, -1000, 1000, 1000, -1000, -1000, -1000};
    std::vector<Data> data;
    std::vector<float> embeddings;
    faiss::idx_t n = 800;
    generate_data(d, centroids, data, embeddings);

    // Load the database
    client->loadDB(400, data.data());

    // Query the full database
    std::vector<Data> results(400);
    for (int i = 0; i < 400; i++) {
        results.push_back(Data());
    }

    client->search(1, embeddings.data(), 400, results.data());

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
    std::vector<Data> data;
    std::vector<float> embeddings;
    faiss::idx_t n = 800;
    generate_data(d, centroids, data, embeddings);

    // Train the index
    core.index->is_trained = true;
    core.train(n, embeddings.data());
    // Load the external db the cache core pulls from
    client->loadDB(400, data.data());

    // Load the first cell
    core.loadCell(0);
    // REQUIRE(core.embeddings.size() == 100);
    REQUIRE(core.data.size() == 100);
    // Load the 4th cell
    core.loadCell(3);
    // REQUIRE(core.embeddings.size() == 200);
    REQUIRE(core.data.size() == 200);


    // Test multiple db queries needed to get full cell (Should need to make 3 total)
    core.nTotal = 100;
    core.loadCell(1);
    // REQUIRE(core.embeddings.size() == 300);
    REQUIRE(core.data.size() == 300);
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
    std::vector<Data> data;
    std::vector<float> embeddings;
    faiss::idx_t n = 800;
    generate_data(d, centroids, data, embeddings);
    std::cout << "Generated data" << std::endl;
    // Train the index
    core.index->is_trained = true;
    core.train(n, embeddings.data());

    // Load the external db the cache core pulls from
    std::cout << "About to load database" << std::endl;
    client->loadDB(400, data.data());
    std::cout << "About to load cell" << std::endl;
    // Load the first cell
    core.loadCell(0);
    std::cout << "Loaded cell" << std::endl;
    size_t xq_n = 1;
    size_t k = 5;
    // TODO: Fix this naming
    // float embeddings_q[k * d];
    std::vector<Data> results(k);
    int cacheHits[n];
    float xq[xq_n];
    xq[0] = centroids[0];
    xq[1] = centroids[1];

    std::cout << "Abotu to search" << std::endl;
    core.search(xq_n, xq, k, results.data(), cacheHits);
    std::cout << "Finished first search" << std::endl;
    REQUIRE(cacheHits[0] == k);
    

    // Make sure all the embeddings come from the first cell
    for (size_t i = 0; i < results.size(); i++) {
        for (size_t j = 0; j < d; j++) { 
            REQUIRE((results[i].embedding[j] <= 105 && results[i].embedding[j] >= 95));
        }
    }

    xq[0] = centroids[6];
    xq[1] = centroids[7];
    core.search(xq_n, xq, k, results.data(), cacheHits);
    // Length is n so there should only be 1
    REQUIRE(cacheHits[0] == -1);

    core.loadCell(3);
    core.search(xq_n, xq, k, results.data(), cacheHits);
    REQUIRE(cacheHits[0] == k);
    // for (int i = 0; i < xq_n * k * d; i++) {
    //     REQUIRE((embeddings_q[i] <= -95 && embeddings_q[i] >= -105));
    // }
    for (size_t i = 0; i < results.size(); i++) {
        for (size_t j = 0; j < d; j++) {
            REQUIRE((results[i].embedding[j] <= -95 && results[i].embedding[j] >= -105));
        }
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
    std::vector<Data> data;
    std::vector<float> embeddings;
    faiss::idx_t n = 800;
    generate_data(d, centroids, data, embeddings);

    // Train the index
    core.index->is_trained = true;
    core.train(n, embeddings.data());

    // Load the external db the cache core pulls from
    client->loadDB(400, data.data());

    // Load the first cell
    core.loadCell(0);

    // Remove the cell
    core.evictCell(0);
    REQUIRE(core.data.size() == 0);
    
    core.loadCell(1);
    core.loadCell(3);
    REQUIRE(core.data.size() == 200);

    size_t xq_n = 1;
    size_t k = 5;
    std::vector<Data> results(k);
    int cacheHits[n];
    float xq[xq_n];
    xq[0] = centroids[6];
    xq[1] = centroids[7];

    core.search(xq_n, xq, k, results.data(), cacheHits);
    REQUIRE(cacheHits[0] == k);
    for (size_t i = 0; i < results.size(); i++) {
        for (size_t j = 0; j < d; j++) {
            REQUIRE((results[i].embedding[j] <= -95 && results[i].embedding[j] >= -105));
        }
    }

    core.evictCell(3);
    core.search(xq_n, xq, k, results.data(), cacheHits);
    REQUIRE(cacheHits[0] == -1);
    REQUIRE(core.data.size() == 100);
    
    core.evictCell(1);
    REQUIRE(core.data.size() == 0);
}
