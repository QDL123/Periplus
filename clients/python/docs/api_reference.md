# Periplus Client Library API Reference

## Introduction

This document provides a detailed API reference for the Periplus client library. It covers all available classes, methods, their parameters, return types, and possible exceptions. This document is intended to be a comprehensive guide for developers integrating the Periplus service into their projects.

---

### Table of Contents
- [Periplus Class](#periplus-class)
  - [Initialization](#initialization)
  - [Methods](#methods)
    - [`initialize`](#initialize)
    - [`train`](#train)
    - [`add`](#add)
    - [`load`](#load)
    - [`search`](#search)
    - [`evict`](#evict)
- [Record NamedTuple](#record-namedtuple)
- [Error Classes](#error-classes)
    - [`PeriplusError`](#peripluserror)
    - [`PeriplusConnectionError`](#periplusconnectionerror)
    - [`PeriplusServerError`](#periplusservererror)

---

## Periplus Class

The `Periplus` class serves as the primary interface to interact with the Periplus service. It manages the connection and provides methods to initialize, train, add data, load data, perform searches, and evict data.

### Initialization

```python
Periplus(host: str, port: int)
```

- **Description**: 
  Initializes the Periplus client, setting up a connection to the specified host and port where the Periplus service is running.

- **Parameters**: 
  - `host` (*str*): The hostname or IP address of the Periplus service.
  - `port` (*int*): The port number on which the Periplus service is running.

- **Example**:
  ```python
  from periplus_client import Periplus

  # Initialize the Periplus client
  client = Periplus(host='localhost', port=8080)
  ```

### Methods

#### `initialize`

```python
async initialize(d: int, db_url: str, options: dict = {}) -> bool
```

- **Description**: 
  Initializes the Periplus instance, preparing it for subsequent operations. This method must be called before any other methods. Subsequent calls to `initialize` will reset the instance.

- **Parameters**:
  - `d` (*int*): Dimensionality of the vector collection (e.g. 128 if your vectors have 128 dimensions).
  - `db_url` (*str*): URL pointing to the endpoint implementing the Periplus database proxy specification.
  - `options` (*dict*, optional): Additional configuration settings.
    - `n_records` (*int*): Estimate of the total number of vectors in the collection. Helps optimize the number of IVF cells.
    - `use_flat` (*bool*): Determines whether to use product quantization (PQ). Defaults to `False`. If `False`, PQ is used for vectors with dimensions ≥ 64 and divisible into subvectors of 8.

- **Returns**: 
  - (*bool*): `True` if the initialization is successful.

- **Raises**:
    - `PeriplusConnectionError`: If the connection to the Periplus service fails.
    - `PeriplusServerError`: If Periplus fails to initialize the instance for any reason.

- **Example**:
  ```python
  success = await client.initialize(
      d=128,
      db_url='http://localhost:5000/proxy',
      options={
          'n_records': 500000,
          'use_flat': False
      }
  )
  if success:
      print("Periplus initialized successfully.")
  ```

---

#### `train`

```python
async train(training_data: List[List[float]]) -> bool
```

- **Description**: 
  Trains the Periplus IVF index using a representative sample of the vector collection. Must be called after `initialize` and before adding any data.

- **Parameters**:
  - `training_data` (*List[List[float]]*): A representative sample of the vector collection. It's recommended to provide 10% of the total collection. Each inner list should have a length equal to `d` specified during initialization.

- **Returns**: 
  - (*bool*): `True` if the training is successful.

- **Raises**:
    - `PeriplusConnectionError`: If the connection to the Periplus service fails.
    - `PeriplusServerError`: If Periplus fails to train the instance for any reason.

- **Example**:
  ```python
  # Sample training data
  sample_training_data = [
      [0.1, 0.2, 0.3, ..., 0.128],
      [0.4, 0.5, 0.6, ..., 0.128]
  ]
  
  success = await client.train(training_data=sample_training_data)
  if success:
      print("Periplus trained successfully.")
  ```

---

#### `add`

```python
async add(ids: List[str], embeddings: List[List[float]]) -> bool
```

- **Description**: 
  Registers new vectors with Periplus. These vectors become available for loading and searching.

- **Parameters**:
  - `ids` (*List[str]*): Unique identifiers corresponding to each vector in `embeddings`.
  - `embeddings` (*List[List[float]]*): List of vector embeddings. Each inner list should have a length equal to `d` specified during initialization.

- **Returns**: 
  - (*bool*): `True` if the data is added successfully.

- **Raises**:
    - `AssertionError`: If the lengths of `ids` and `embeddings` do not match.
    - `PeriplusConnectionError`: If the connection to the Periplus service fails.
    - `PeriplusServerError`: If Periplus fails to complete the **add** operation for any reason.

- **Example**:
  ```python
  # Vector data
  vector_ids = ['vec1', 'vec2']
  vector_embeddings = [
      [0.1, 0.2, 0.3, ..., 0.128],
      [0.4, 0.5, 0.6, ..., 0.128]
  ]
  
  success = await client.add(ids=vector_ids, embeddings=vector_embeddings)
  if success:
      print("Vectors added to Periplus successfully.")
  ```

---

#### `load`

```python
async load(xq: List[float], options: dict = {}) -> bool
```

- **Description**: 
  Loads one or more IVF cells from the vector database based on the provided vector. This prepares the relevant data for efficient querying.

- **Parameters**:
  - `xq` (*List[float]*): A vector indicating which IVF cell(s) to load. The cells corresponding to the nearest centroids to `xq` will be loaded.
  - `options` (*dict*, optional): Additional loading options.
    - `n_load` (*int*): Number of IVF cells to load. Defaults to `1`.

- **Returns**: 
  - (*bool*): `True` if the cells are loaded successfully.

- **Raises**:
    - `PeriplusConnectionError`: If the connection to the Periplus service fails.
    - `PeriplusServerError`: If Periplus fails to complete the **load** operation for any reason.

- **Example**:
  ```python
  query_vector = [0.1, 0.2, 0.3, ..., 0.128]
  
  success = await client.load(xq=query_vector, options={'n_load': 3})
  if success:
      print("Cells loaded into Periplus successfully.")
  ```

---

#### `search`

```python
async search(k: int, xq: List[List[float]], options: dict = {}) -> List[List[Record]]
```

- **Description**: 
  Performs a search for the `k` nearest neighbors for each query vector provided. Returns results only if the relevant sections of the index are loaded.

- **Parameters**:
  - `k` (*int*): Number of nearest neighbors to return for each query vector.
  - `xq` (*List[List[float]]*): List of query vectors. Each inner list should have a length equal to `d` specified during initialization.
  - `options` (*dict*, optional): Additional search options.
    - `n_probe` (*int*): Number of IVF cells to search for nearest neighbors. Defaults to `1`.
    - `require_all` (*bool*): Determines if all relevant IVF cells must be loaded for a cache hit. Defaults to `True`.

- **Returns**: 
  - (*List[List[Record]]*): A list where each element corresponds to the results for a query vector. Each result is a list of `Record` namedtuples containing `id`, `embedding`, `document`, and `metadata`. If a query results in a cache miss, the corresponding list will be empty.

- **Raises**:
    - `PeriplusConnectionError`: If the connection to the Periplus service fails.
    - `PeriplusServerError`: If Periplus fails to complete the **search** operation for any reason.

- **Example**:
  ```python
  query_vectors = [
      [0.1, 0.2, 0.3, ..., 0.128],
      [0.5, 0.6, 0.7, ..., 0.628]
  ]

    results = await client.search(
        k=5,
        xq=query_vectors,
        options={
            'n_probe': 2,
            'require_all': False
        }
    )

    for i, query_result in enumerate(results):
        print(f"Results for Query Vector {i+1}:")
        for record in query_result:
            print(f"ID: {record.id}, Record: {record.document}, Metadata: {record.metadata}")

---

#### `evict`

```python
async evict(vector: List[float], options: dict = {}) -> bool
```

- **Description**: 
  Evicts one or more IVF cells from Periplus's in-memory cache, freeing up resources.
- **Parameters**:
  - `vector` (*List[float]*): A vector indicating which IVF cell(s) to evict. The cells corresponding to the nearest centroids to `vector` will be evicted.
  - `options` (*dict*, optional): Additional eviction options.
    - `n_evict` (*int*): Number of IVF cells to evict. Defaults to `1`.

- **Returns**: 
  - (*bool*): `True` if the cells are evicted successfully.

- **Raises**:
    - `PeriplusConnectionError`: If the connection to the Periplus service fails.
    - `PeriplusServerError`: If Periplus fails to complete the **evict** operation for any reason.

- **Example**:
  ```python
  evict_vector = [0.1, 0.2, 0.3, ..., 0.128]
  
  success = await client.evict(vector=evict_vector, options={'n_evict': 2})
  if success:
      print("Cells evicted from Periplus successfully.")
  ```

---

## Record NamedTuple

```python
Record = namedtuple('Record', ['id', 'embedding', 'document', 'metadata'])
```

- **Description**: 
  Represents a record retrieved from the Periplus service.

- **Attributes**:
  - `id` (*str*): Unique identifier of the record.
  - `embedding` (*List[float]*): Vector representation of the document.
  - `document` (*str*): Content of the original document.
  - `metadata` (*str*): Additional metadata associated with the record.

- **Example**:
  ```python
  # Assuming 'record' is an instance of Record
  print(f"ID: {record.id}")
  print(f"Embedding: {record.embedding}")
  print(f"Document: {record.document}")
  print(f"Metadata: {record.metadata}")
  ```

---
Certainly! Here’s the new **Error Classes** section that you can add to your `api_reference.md`:

---

### Error Classes

#### `PeriplusError`

```python
class PeriplusError(Exception):
    """Base exception for all Periplus-related errors."""
```

- **Description**: 
  The base exception class for all errors related to Periplus. Other error classes inherit from this base class.

#### `PeriplusConnectionError`

```python
class PeriplusConnectionError(PeriplusError):
    """Raised for connection-related errors when interacting with Periplus."""
```

- **Description**: 
  This exception is raised when a connection-related error occurs when connecting to Periplus.
  
- **Attributes**:
  - `address` (*str*, optional): The address that was being connected to when the error occurred.
  - `port` (*int*, optional): The port that was being used for the connection.

- **Example**:
  ```python
  raise PeriplusConnectionError(
      message="Failed to connect to Periplus server.",
      address="localhost",
      port=8080
  )
  ```

#### `PeriplusServerError`

```python
class PeriplusServerError(PeriplusError):
    """Raised when an operation fails to complete within Periplus."""
```

- **Description**: 
  This exception is raised when an error within Periplus prevents an operation from completing successfully.
  
- **Attributes**:
  - `operation` (*str*, optional): The operation that was being performed when the error occurred.
  - `error_code` (*int*, optional): The error code returned by the server.

- **Example**:
  ```python
  raise PeriplusServerError(
      message="Server failed to execute the operation.",
      operation="initialize",
      error_code=500
  )
  ```

---