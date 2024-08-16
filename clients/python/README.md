# Periplus Client Library

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

## Introduction

The Periplus client library provides an interface to interact with a Periplus instance. Periplus is a remote vector database cache. This client library allows you to initialize, train, add data, load data, perform searches, and evict data within Periplus. For more information on Periplus, please refer to the [README.md](../../README.md).

## Installation

### Prerequisites

- Python 3.8+
- `struct` and `collections` libraries (included in the Python standard library)

### Installation via pip

You can install the Periplus client library using pip:

```bash
pip install periplus-client
```

## Quick Start

Here’s a quick example of how to use the Periplus client library to interact with the Periplus service:

```python
from periplus_client import Periplus

# Initialize the Periplus client
client = Periplus(host='localhost', port=8080)

# Initialize the Periplus instance
await client.initialize(
    d=128,
    db_url='http://localhost:5000/proxy',
    options={'n_records': 500000, 'use_flat': False}
)

# Train the instance with sample data
await client.train(training_data=[[0.1, 0.2, ..., 0.128]])

# Add data to the instance
await client.add(ids=['vec1', 'vec2'], embeddings=[[0.1, 0.2, ..., 0.128]])

# Perform a search
results = await client.search(k=5, xq=[[0.1, 0.2, ..., 0.128]])
print(results)
```

## Documentation

### API Reference

For a complete API reference, including details on all classes, methods, and their usage, please refer to the [API Reference](docs/api_reference.md).

## Contributing

Contributions are welcome! If you'd like to contribute to the project, please follow these steps:

1. Fork the repository.
2. Create a new branch: `git checkout -b feature/your-feature-name`.
3. Make your changes.
4. Commit your changes: `git commit -m 'Add some feature'`.
5. Push to the branch: `git push origin feature/your-feature-name`.
6. Open a pull request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---