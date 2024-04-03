#include <iostream>
// #include "cache.h"
#include "db_client.h"


int main() {
    size_t d = 10;
    // Cache cache(d);
    std::cout << "Construct client" << std::endl;
    DBClient client(d);

    faiss::idx_t n = 10;
    float data[d * n];

    // populate data
    for (int i = 0; i < n * d; i++) {
        data[i] = float(i);
    }

    std::cout << "Load the database" << std::endl;
    client.loadDB(n, data);

    float xq[d];
    for (int i = 0; i < d; i++) {
        xq[i] = i;
    }

    faiss::idx_t k = 2;
    float x[d * k];
    std::cout << "Search" << std::endl;
    client.search(1, xq, 2, x);

    std::cout << "Output the results" << std::endl;
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < d; j++) {
            std::cout << x[i * d + j];
        }
        std::cout << std::endl;
    }

    return 0;
}

