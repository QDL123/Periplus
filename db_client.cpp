#include "db_client.h"
#include "data.h" 

#include <curl/curl.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <iostream>
#include <faiss/IndexFlat.h>

// Callback function to handle the data received from the server
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


DBClient::DBClient(size_t d, std::shared_ptr<char> db_url) {
    this->d = d;
    this->size = 0;
    this->db_url = db_url;
}

// Function to construct the query string for the array of floats
std::string constructFloatArrayQuery(const float* array, size_t length) {
    std::ostringstream oss;
    for(size_t i = 0; i < length; ++i) {
        oss << "&xq=";
        oss << array[i];
    }
    return oss.str();
}

void DBClient::search(faiss::idx_t n, float *xq, faiss::idx_t k, Data *x) {
    // TODO: GUARD AGAINST Querying greater than the size of the database
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    // Initialize CURL
    curl = curl_easy_init();
    if(curl) {
        // Construct the query parameters
        std::ostringstream urlStream;
        urlStream << this->db_url.get() << "?n=" << k << constructFloatArrayQuery(xq, this->d);

        // Get the complete URL
        std::string url = urlStream.str();

        // Set the URL for the request
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);


        // Set the callback function to handle the response data
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        // Set the user data parameter (string to store the response data)
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request, res will get the return code
        res = curl_easy_perform(curl);

        // Check for errors
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Parse the JSON response using RapidJSON
            rapidjson::Document document;
            document.Parse(readBuffer.c_str());

            if (document.HasParseError()) {
                std::cerr << "Parse error: " << document.GetParseError() << std::endl;
            } else if (document.HasMember("results") && document["results"].IsArray()) {
                size_t i = 0;
                for (const auto& item : document["results"].GetArray()) {
                    if (item.HasMember("id") && item["id"].IsString()) {
                        const char* id_str = item["id"].GetString();
                        x[i].id_len = std::strlen(id_str);
                        x[i].id = std::shared_ptr<char>(new char[x[i].id_len + 1]);
                        std::strcpy(x[i].id.get(), id_str);
                    }
                    if (item.HasMember("embedding") && item["embedding"].IsArray()) {
                        x[i].embedding_len = item["embedding"].Size();
                        x[i].embedding = std::shared_ptr<float>(new float[x[i].embedding_len]);
                        size_t index = 0;
                        for (const auto& val : item["embedding"].GetArray()) {
                            if (val.IsFloat()) {
                                x[i].embedding[index++] = val.GetFloat();
                            }
                        }
                    }
                    if (item.HasMember("document") && item["document"].IsString()) {
                        const char* document_str = item["document"].GetString();
                        x[i].document_len = std::strlen(document_str);
                        x[i].document = std::shared_ptr<char>(new char[x[i].document_len + 1]);
                        std::strcpy(x[i].document.get(), document_str);
                    }
                    if (item.HasMember("metadata") && item["metadata"].IsString()) {
                        const char* metadata_str = item["metadata"].GetString();
                        x[i].metadata_len = std::strlen(metadata_str);
                        x[i].metadata = std::shared_ptr<char>(new char[x[i].metadata_len + 1]);
                        std::strcpy(x[i].metadata.get(), metadata_str);
                    }

                    i++;
                }
            } else {
                std::cout << "JSON wasn't an array" << std::endl;
            }
        }

        // Cleanup
        curl_easy_cleanup(curl);
    }
}



// MOCK DB Client Implementation //////

DBClient_Mock::DBClient_Mock(size_t d) : DBClient(d, std::shared_ptr<char>(nullptr)) {
    this->index = std::unique_ptr<faiss::Index>(new faiss::IndexFlatL2(this->d));
}

void DBClient_Mock::loadDB(faiss::idx_t n, Data *data) {
    // Add data to the index 
    float *embeddings = new float[n * this->d];
    for (size_t i = 0; i < n; i++) {
        std::memcpy(&embeddings[this->d * i], data[i].embedding.get(), sizeof(float) * this->d);
    }

    this->index->add(n, embeddings);
    delete[] embeddings;
    // Save the data in the 
    this->data = std::unique_ptr<Data[]>(new Data[n]);
    for (int i = 0; i < n; i++) {
        this->data[i] = std::move(data[i]);
    }
    this->size = n;
}


void DBClient_Mock::search(faiss::idx_t n, float *xq, faiss::idx_t k, Data *x) {
    // TODO: GUARD AGAINST Querying greater than the size of the database
    float distances[n * k];
    faiss::idx_t labels[n * k];
    this->index->search(n, xq, k, distances, labels);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < k; j++) {
            faiss::idx_t index = labels[j + i * k];
            Data copiedStruct(this->data[index]);
            x[j + i * k] = Data(this->data[labels[j + i * k]]);
        }
    }
}