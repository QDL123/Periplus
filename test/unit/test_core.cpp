#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "../../src/data.h"
#include "../../src/core.h"
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
            int id_num = (i * 100) + j;
            std::string id_string = std::to_string(id_num);
            std::vector<char> id(id_string.begin(), id_string.end());
            id.push_back('\0');
            
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

            data.push_back(Data(id.size(), d, 3, 4, id.data(), embedding, document, metadata));
        }
    }
}


TEST_CASE("Construct Cache Core", "[Core]") {
    size_t d = 10;
    float nTotal = 10000;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient_Mock(d));
    Core core(d, client, 100, nTotal, false);

    REQUIRE(core.d == d);
}


TEST_CASE("Train the cache core", "[Core::train]") {
    // Create the cache core
    size_t d = 10;
    float nTotal = 10000;
    std::shared_ptr<DBClient> client = std::shared_ptr<DBClient>(new DBClient_Mock(d));
    Core core(d, client, 4, nTotal, false);

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

TEST_CASE("Load Cell", "[Core::loadCell]") {
    // Create cache core
    size_t d = 2;
    float nTotal = 800;
    std::shared_ptr<DBClient_Mock> client = std::shared_ptr<DBClient_Mock>(new DBClient_Mock(d));
    size_t nCells = 4;
    Core core(d, client, nCells, nTotal, false);

    // Manually set the centroids for testing purposes
    const float centroids[] = {100, 100, -100, 100, 100, -100, -100, -100};
    core.quantizer->add(nCells, centroids);

    // Generate dataset
    std::vector<Data> data;
    std::vector<float> embeddings;
    faiss::idx_t n = 800;
    generate_data(d, centroids, data, embeddings);

    std::vector<std::shared_ptr<char[]>> ids;
    for (auto itr = data.begin(); itr != data.end(); itr++) {
        ids.push_back(std::shared_ptr<char[]>(new char[itr->id_len]));
        std::memcpy(ids[ids.size() - 1].get(), itr->id.get(), sizeof(char) * (itr->id_len));
    }

    std::shared_ptr<float[]> embeddings_copy(new float[embeddings.size()]);
    memcpy(embeddings_copy.get(), embeddings.data(), sizeof(float) * embeddings.size());
    core.add(data.size(), ids, embeddings_copy);

    // Train the index
    core.index->is_trained = true;
    core.train(n, embeddings.data());
    // Load the external db the cache core pulls from

    client->loadDB(400, data.data());

    // Load the first cell
    core.loadCell(0);
    REQUIRE(core.data_map.size() == 100);
    // Load the 4th cell
    core.loadCell(3);
    // REQUIRE(core.embeddings.size() == 200);
    REQUIRE(core.data_map.size() == 200);


    // Test multiple db queries needed to get full cell (Should need to make 3 total)
    core.nTotal = 100;
    core.loadCell(1);
    // REQUIRE(core.embeddings.size() == 300);
    REQUIRE(core.data_map.size() == 300);
    // Is there a way to assert how many times db->search is being called to validate what we're testing?
}


TEST_CASE("Search", "[Core::search]") {
    // Create cache core
    size_t d = 2;
    float nTotal = 800;
    std::shared_ptr<DBClient_Mock> client = std::shared_ptr<DBClient_Mock>(new DBClient_Mock(d));
    size_t nCells = 4;
    Core core(d, client, nCells, nTotal, false);

    // Manually set the centroids for testing purposes
    const float centroids[] = {100, 100, -100, 100, 100, -100, -100, -100};
    core.quantizer->add(nCells, centroids);

    // Generate dataset
    std::vector<Data> data;
    std::vector<float> embeddings;
    faiss::idx_t n = 800;
    generate_data(d, centroids, data, embeddings);

    std::vector<std::shared_ptr<char[]>> ids;
    for (auto itr = data.begin(); itr != data.end(); itr++) {
        ids.push_back(std::shared_ptr<char[]>(new char[itr->id_len]));
        std::memcpy(ids[ids.size() - 1].get(), itr->id.get(), sizeof(char) * (itr->id_len));
    }

    std::shared_ptr<float[]> embeddings_copy(new float[embeddings.size()]);
    memcpy(embeddings_copy.get(), embeddings.data(), sizeof(float) * embeddings.size());
    core.add(data.size(), ids, embeddings_copy);

    // Train the index
    core.index->is_trained = true;
    core.train(n, embeddings.data());

    // Load the external db the cache core pulls from
    client->loadDB(400, data.data());

    // Load the first cell
    core.loadCell(0);
    size_t xq_n = 1;
    size_t k = 5;
    std::vector<Data> results(k);
    int cacheHits[n];
    float xq[xq_n * d];
    xq[0] = centroids[0];
    xq[1] = centroids[1];

    size_t nprobe = 1;
    bool require_all = true;
    core.search(xq_n, xq, k, nprobe, require_all, results.data(), cacheHits);
    REQUIRE(cacheHits[0] == k);
    

    // Make sure all the embeddings come from the first cell
    for (size_t i = 0; i < results.size(); i++) {
        for (size_t j = 0; j < d; j++) { 
            REQUIRE((results[i].embedding[j] <= 105 && results[i].embedding[j] >= 95));
        }
    }

    xq[0] = centroids[6];
    xq[1] = centroids[7];
    core.search(xq_n, xq, k, nprobe, require_all, results.data(), cacheHits);
    // Length is n so there should only be 1
    REQUIRE(cacheHits[0] == -1);

    core.loadCell(3);
    core.search(xq_n, xq, k, nprobe, require_all, results.data(), cacheHits);
    REQUIRE(cacheHits[0] == k);
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
    std::shared_ptr<DBClient_Mock> client = std::shared_ptr<DBClient_Mock>(new DBClient_Mock(d));
    size_t nCells = 4;
    Core core(d, client, nCells, nTotal, false);

    // Manually set the centroids for testing purposes
    const float centroids[] = {100, 100, -100, 100, 100, -100, -100, -100};
    core.quantizer->add(nCells, centroids);

    // Generate dataset
    std::vector<Data> data;
    std::vector<float> embeddings;
    faiss::idx_t n = 800;
    generate_data(d, centroids, data, embeddings);

    std::vector<std::shared_ptr<char[]>> ids;
    for (auto itr = data.begin(); itr != data.end(); itr++) {
        ids.push_back(std::shared_ptr<char[]>(new char[itr->id_len]));
        std::memcpy(ids[ids.size() - 1].get(), itr->id.get(), sizeof(char) * (itr->id_len));
    }

    std::shared_ptr<float[]> embeddings_copy(new float[embeddings.size()]);
    memcpy(embeddings_copy.get(), embeddings.data(), sizeof(float) * embeddings.size());
    core.add(data.size(), ids, embeddings_copy);

    // Train the index
    core.index->is_trained = true;
    core.train(n, embeddings.data());

    // Load the external db the cache core pulls from
    client->loadDB(400, data.data());

    // Load the first cell
    core.loadCell(0);

    // Remove the cell
    core.evictCell(0);
    REQUIRE(core.data_map.size() == 0);
    
    core.loadCell(1);
    core.loadCell(3);
    REQUIRE(core.data_map.size() == 200);

    size_t xq_n = 1;
    size_t k = 5;
    std::vector<Data> results(k);
    int cacheHits[n];
    float xq[xq_n * d];
    xq[0] = centroids[6];
    xq[1] = centroids[7];

    size_t nprobe = 1;
    bool require_all = true;
    core.search(xq_n, xq, k, nprobe, require_all, results.data(), cacheHits);
    REQUIRE(cacheHits[0] == k);
    for (size_t i = 0; i < results.size(); i++) {
        for (size_t j = 0; j < d; j++) {
            REQUIRE((results[i].embedding[j] <= -95 && results[i].embedding[j] >= -105));
        }
    }

    core.evictCell(3);
    core.search(xq_n, xq, k, nprobe, require_all, results.data(), cacheHits);
    REQUIRE(cacheHits[0] == -1);
    REQUIRE(core.data_map.size() == 100);
    
    core.evictCell(1);
    REQUIRE(core.data_map.size() == 0);
}
