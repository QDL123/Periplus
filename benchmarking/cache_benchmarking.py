import numpy as np
import struct
import time
import asyncio
from clients.python.client import CacheClient


# Function to read fvecs file
def read_fvecs(file_path):
    with open(file_path, 'rb') as f:
        while True:
            dim_bytes = f.read(4)
            if not dim_bytes:
                break
            dim = struct.unpack('i', dim_bytes)[0]
            vector = np.fromfile(f, dtype=np.float32, count=dim)
            yield vector

# Function to read ivecs file
def read_ivecs(file_path):
    with open(file_path, 'rb') as f:
        while True:
            dim_bytes = f.read(4)
            if not dim_bytes:
                break
            dim = struct.unpack('i', dim_bytes)[0]
            vector = np.fromfile(f, dtype=np.int32, count=dim)
            yield vector

async def main():
    # Paths to your files
    query_file_path = './datasets/sift/sift_query.fvecs'
    groundtruth_file_path = './datasets/sift/sift_groundtruth.ivecs'
    training_file_path = './datasets/sift/sift_learn.fvecs'
    base_file_path = './datasets/sift/sift_base.fvecs'

    # Read queries and groundtruths
    print("reading queries")
    queries = list(read_fvecs(query_file_path))
    print("reading groundtruth")
    groundtruths = list(read_ivecs(groundtruth_file_path))

    # Ensure the number of queries and groundtruths match
    assert len(queries) == len(groundtruths), "The number of queries does not match the number of groundtruth entries."

    print("reading traning data")
    training = list(read_fvecs(training_file_path))
    print("reading base")
    base = list(read_fvecs(base_file_path))


    # Initialize the cache
    d = 128
    url = 'http://localhost:8000/v1/pinecone'
    nTotal = 1000000
    client = CacheClient("localhost", 13)
    
    print("initializing")
    await client.initialize(d=d, db_url=url, options={ 'nTotal': nTotal })

    print("training")
    await client.train(training_data=training)

    print("Adding")
    # Probably going to need to modify this to be more scalable
    await client.add(ids=range(len(base)), embeddings=base)

    print("Cache is in READY state")


    # Initialize metrics
    recalls = []
    latencies = []

    # Perform queries and calculate recall and latency
    for i, query in enumerate(queries):
        # if (i < 100):
        #     continue
        print("performing query: " + str(i))
        groundtruth = groundtruths[i][:10]
        top_k = 10

        await client.load(vector=query.tolist())

        # Record the start time
        start_time = time.time()
        
        # Query the cache
        response = await client.search(k=top_k, query_vectors=[query.tolist()])
        # (should always be a hit since we are loading beforehand)
        assert len(response[0]) > 0
        
        # Record the end time
        end_time = time.time()

        await client.evict(vector=query.tolist())



        # Calculate latency
        latency = end_time - start_time
        if i % 10 == 0:
            print("latency: " + str(latency))
        latencies.append(latency)

        # Extract Cache IDs
        cache_ids = [int(match.id) for match in response[0]]
        
        # Calculate recall
        correct = len(set(cache_ids).intersection(set(groundtruth)))
        recall = correct / top_k
        if i % 10 == 0:
            print("Recall: " + str(recall))
        recalls.append(recall)

        if i == 500:
            break

    # Calculate recall distribution and overall recall
    recall_distribution = np.histogram(recalls, bins=10, range=(0, 1))
    overall_recall = np.mean(recalls)

    # Calculate latency distribution and average/median latency
    latency_distribution = np.histogram(latencies, bins=10)
    average_latency = np.mean(latencies)
    median_latency = np.median(latencies)

    # Print results
    print("Recall Distribution (bins, counts):", recall_distribution)
    print("Overall Recall:", overall_recall)
    print("Latency Distribution (bins, counts):", latency_distribution)
    print("Average Latency:", average_latency)
    print("Median Latency:", median_latency)


if __name__ == '__main__':
    asyncio.run(main())
