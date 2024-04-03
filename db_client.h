#include <faiss/IndexFlat.h>

struct DBClient {
    size_t d;
    std::unique_ptr<faiss::Index> index;
    std::unique_ptr<float[]> data;

    DBClient(size_t d);

    void loadDB(faiss::idx_t n, float* data);

    void search(faiss::idx_t n, float *xq, faiss::idx_t k, float *x);
};