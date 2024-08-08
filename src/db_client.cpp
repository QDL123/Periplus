#include "db_client.h"
#include "data.h" 

#include <iostream>
#include <memory>

#include <curl/curl.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <faiss/IndexFlat.h>
#include <cpr/cpr.h>

// Callback function to handle the data received from the server
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


DBClient::DBClient(size_t d, std::shared_ptr<char[]> db_url) {
    this->d = d;
    this->size = 0;
    this->db_url = db_url;
}

DBClient::~DBClient() {}


// Function to construct the JSON body from a vector of strings
std::string constructJsonBody(const std::vector<std::string>& ids) {
    rapidjson::Document d;
    d.SetObject();
    rapidjson::Document::AllocatorType& allocator = d.GetAllocator();

    rapidjson::Value idArray(rapidjson::kArrayType);
    for (const auto& id : ids) {
        idArray.PushBack(rapidjson::Value().SetString(id.c_str(), allocator), allocator);
    }

    d.AddMember("ids", idArray, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);

    return buffer.GetString();
}


void DBClient::search(std::vector<std::string> ids, Data *x) {

    // Get the url
    std::ostringstream urlStream;
    urlStream << this->db_url.get();
    std::string url = urlStream.str();

    // Get the body
    std::string jsonBody = constructJsonBody(ids);


    // Make a POST request
    this->session.SetUrl(cpr::Url{url});
    this->session.SetHeader(cpr::Header{{"Content-Type", "application/json"}});
    this->session.SetBody(cpr::Body{jsonBody});
    auto response = this->session.Post();

    if (response.status_code != 200) {
        std::cerr << "Request response status: " << response.status_code << "\n";
        std::cerr << "response.text: " << response.text << std::endl;
        std::cerr << "Request failed. Error: " << response.error.message << std::endl;

        // TODO: handle this gracefully
        throw;
    } else {
        rapidjson::Document document;
        document.Parse(response.text.c_str());

        if (document.HasParseError()) {
            std::cerr << "Parse error: " << document.GetParseError() << std::endl;
        } else if (document.HasMember("results") && document["results"].IsArray()) {
            size_t i = 0;
            for (const auto& item : document["results"].GetArray()) {
                if (item.HasMember("id") && item["id"].IsString()) {
                    const char* id_str = item["id"].GetString();
                    x[i].id_len = std::strlen(id_str);
                    x[i].id = std::shared_ptr<char[]>(new char[x[i].id_len + 1]);
                    std::strcpy(x[i].id.get(), id_str);
                }
                if (item.HasMember("embedding") && item["embedding"].IsArray()) {
                    x[i].embedding_len = item["embedding"].Size();
                    x[i].embedding = std::shared_ptr<float[]>(new float[x[i].embedding_len]);
                    size_t index = 0;
                    for (const auto& val : item["embedding"].GetArray()) {
                        if (val.IsFloat()) {
                            x[i].embedding[index++] = val.GetFloat();
                        }
                    }
                    assert(x[i].embedding.get() != nullptr);
                } else {
                    std::cerr << "document has no embedding associated with it" << std::endl;
                    assert(x[i].embedding.get() != nullptr);
                }
                if (item.HasMember("document") && item["document"].IsString()) {
                    const char* document_str = item["document"].GetString();
                    x[i].document_len = std::strlen(document_str);
                    x[i].document = std::shared_ptr<char[]>(new char[x[i].document_len + 1]);
                    std::strcpy(x[i].document.get(), document_str);
                }
                if (item.HasMember("metadata") && item["metadata"].IsString()) {
                    const char* metadata_str = item["metadata"].GetString();
                    x[i].metadata_len = std::strlen(metadata_str);
                    x[i].metadata = std::shared_ptr<char[]>(new char[x[i].metadata_len + 1]);
                    std::strcpy(x[i].metadata.get(), metadata_str);
                }

                assert(x[i].embedding.get() != nullptr);
                i++;
            }
        } else {
            std::cout << "JSON wasn't an array" << std::endl;
        }
    }
}


////////////////////////////////////////////////////////
// MOCK DB Client Implementation (Unit Testing Purposes)
////////////////////////////////////////////////////////

DBClient_Mock::DBClient_Mock(size_t d) : DBClient(d, std::shared_ptr<char[]>(nullptr)) {
    this->data_map = std::unordered_map<std::string, Data>();
}

void DBClient_Mock::loadDB(faiss::idx_t n, Data *data) {
    for (int i = 0; i < n; i++) {
        std::string id(data[i].id.get());
        Data data_entry = data[i];
        this->data_map.insert({id, data_entry});
    }
    this->size = n;
}

void DBClient_Mock::search(std::vector<std::string> ids, Data *x) {
    for (size_t i = 0; i < ids.size(); i++) {
        new(&x[i]) Data(this->data_map[ids[i]]);
    }
}