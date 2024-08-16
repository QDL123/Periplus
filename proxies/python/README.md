# Periplus Proxy
![MIT License](https://img.shields.io/badge/License-MIT-yellow.svg)

## Introduction

**Periplus Proxy** is a Python package designed to make it easy to implement the webhook interface that Periplus expects for loading data. It does so by using a dependency injection pattern where a handler function for fetching data is implemented by the user and passed to a templated controller class. This allows the user to only worry about implementing a function that takes a list of ids and returns the associated records. The package takes care of running the server with the correct webhook interface. Using this package is not required to use Periplus. Any webhook implementation that adheres to the specification will work.

## Installation

You can install the package using `pip`:

```bash
pip install periplus-proxy
```

## Usage

### 1. Setting Up Your Webhook
Here's a simple example that shows how to use the package to set up your webhook:

```python
from periplus_proxy import ProxyController, Query, QueryResult, Record

# 1. Implement a function that adheres to the interface shown here.
async def fetch_ids(request: Query) -> QueryResult:
    ids = request.ids

    data = # Implement logic to fetch data with the ids. (data should be List[Record])

    return QueryResult(results=data)

# 2. Initialize a ProxyController instance with the endpoint and handler function.
controller = ProxyController(endpoint='/api/fetch-data', fetch_ids=fetch_ids)

# 3. Run the server.
controller.run()
```

Here's a slightly more complex example for fetching data from Pinecone:
```python
from periplus_proxy import ProxyController, Query, QueryResult, Record
from pinecone import Pinecone

pc = Pinecone(api_key='<PINECONE-API-KEY>')
index_name = '<PINECONE-INDEX-NAME>'
index = pc.Index(index_name)

def split_list(lst, chunk_size=100):
    return [lst[i:i + chunk_size] for i in range(0, len(lst), chunk_size)]

async def fetch_ids(request: Query) -> QueryResult:
    ids = request.ids
    results = []
    id_lists = split_list(ids)
    for ids in id_lists:
        response = index.fetch(ids=ids)
        vectors = response['vectors']
        for id, data in vectors.items():
            results.append(Record(embedding=data['values'], document="", id=id, metadata=data['metadata']))

    return QueryResult(results=results)

if __name__ == "__main__":
    endpoint = "/api/v1/load_data"
    controller = ProxyController(endpoint=endpoint, fetch_ids=fetch_ids)
    controller.run(host='localhost', port=3000)
```

### 2. Running the Webhook

To start the server and make the webhook available, run your program:

```bash
python your_program.py
```

By default, the server will start on `0.0.0.0` at port `8000`. You can customize the host and port by passing host and port arguments to the `run()` method as shown above.

### 3. Making a Request

You can send a POST request to your webhook with a JSON payload containing the IDs of the vectors you want to fetch:

```json
{
  "ids": ["id1", "id2", "id3"]
}
```

### 4. Example Response

The response will be a JSON object containing the fetched data:

```json
{
  "results": [
    {
      "embedding": [0.1, 0.2, 0.3],
      "document": "document",
      "id": "id1",
      "metadata": "{}"
    },
    {
      "embedding": [0.4, 0.5, 0.6],
      "document": "document",
      "id": "id2",
      "metadata": "{}"
    },
    {
      "embedding": [0.7, 0.8, 0.9],
      "document": "document",
      "id": "id3",
      "metadata": "{}"
    }
  ]
}
```

## Webhook Specification

### Endpoint

- **URL:** `/api/v1/load_data` (This is flexible so long as the endpoint is correctly specified when initializing the Periplus instance)
- **Method:** POST
- **Content-Type:** `application/json`

### Request Payload

The webhook expects a JSON payload with the following structure:

```json
{
  "ids": ["id1", "id2", "id3"]
}
```

- **ids:** An array of strings, where each string represents the ID of a vector stored in the database.

### Response Payload

The response will be a JSON object containing the fetched data associated with the provided IDs:

```json
{
  "results": [
    {
      "embedding": [0.1, 0.2, 0.3],
      "document": "document",
      "id": "id1",
      "metadata": "{}"
    },
    {
      "embedding": [0.4, 0.5, 0.6],
      "document": "document",
      "id": "id2",
      "metadata": "{}"
    }
  ]
}
```

- **results:** An array of objects where each object represents the data associated with an ID.
  - **embedding:** A list of floats representing the vector's embedding.
  - **document:** A string that could represent additional information or a document associated with the vector.
  - **id:** The ID of the vector.
  - **metadata:** A string containing metadata associated with the vector.

### Error Responses

- **422 Unprocessable Entity:** If the incoming payload does not match the expected structure, the webhook will return a `422` status code with details about the validation errors.
- **500 Internal Server Error:** If there is an issue with the handler function or the fetched data, the webhook will return a `500` status code with an appropriate error message.


## Requirements

- Python 3.8+
- FastAPI
- Pydantic
- Uvicorn
- Pinecone Python Client

## Contributing
We welcome contributions to the Periplus Proxy! To contribute:

- Fork the repository.
- Create a new branch: git checkout -b feature/your-feature-name.
- Make your changes.
- Commit your changes: git commit -m 'Add some feature'.
- Push to the branch: git push origin feature/your-feature-name.
- Open a pull request.

## License

This project is licensed under the MIT License.
