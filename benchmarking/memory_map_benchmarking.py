import numpy as np
import struct
import time
import asyncio
import faiss
import math

def read_fvecs_chunks(file_path, n):
    with open(file_path, 'rb') as f:
        while True:
            vectors = []
            for _ in range(n):
                # Read dimension (4 bytes)
                dim_bytes = f.read(4)
                if not dim_bytes:
                    if vectors:
                        yield np.array(vectors, dtype=np.float32)
                    return
                dim = struct.unpack('i', dim_bytes)[0]
                
                # Read the vector data
                vector = np.fromfile(f, dtype=np.float32, count=dim)
                vectors.append(vector)
            
            yield np.array(vectors, dtype=np.float32)


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


def train(index):
    # Training data should only exist in RAM during execution
    # of this function
    training_file_path = './datasets/sift/sift_learn.fvecs'
    training = list(read_fvecs(training_file_path))
    index.train(np.array(training))


async def main():
    # Initialize the index
    d = 128
    nlist = int(2 * math.sqrt(1000000))
    quantizer = faiss.IndexFlatL2(d)
    index = faiss.IndexIVFFlat(quantizer, d, nlist)

    # Train the index
    print("Training")
    train(index)

    print("Writing index to disk and setting up memory mapping")
    # Store the index to disk
    faiss.write_index(index, 'faiss_index.ivf')

    # Load the index from disk with mmap
    index = faiss.read_index('faiss_index.ivf')

    # Add data to the index in chunks (It's memory mapped so the file system
    # is being used to expand the virtual address space)
    print("Adding data to the index")
    base_file_path = './datasets/sift/sift_base.fvecs'
    i = 0
    for vectors in read_fvecs_chunks(base_file_path, 10000):
        print("Read a chunk: " + str(i))
        i += 1
        index.add(vectors)
    
    # Save the updated index to disk
    faiss.write_index(index, 'updated_index.ivf')

    # Initialize metrics
    recalls = []
    latencies = []

    # Perform queries and calculate recall and latency
    print("REading ground truth data")
    groundtruth_file_path = './datasets/sift/sift_groundtruth.ivecs'
    groundtruths = list(read_ivecs(groundtruth_file_path))

    print("Reading query data")
    query_file_path = './datasets/sift/sift_query.fvecs'
    queries = list(read_fvecs(query_file_path))
    print("Performing queries")
    for i, query in enumerate(queries):
        print("performing query: " + str(i))
        groundtruth = groundtruths[i][:10]
        top_k = 10

        # Record the start time
        start_time = time.time()
        
        # Query the cache
        _, indices = index.search(np.array([query.tolist()]), top_k)
        
        # Record the end time
        end_time = time.time()

        # Calculate latency
        latency = end_time - start_time
        if i % 10 == 0:
            print("latency: " + str(latency))
        latencies.append(latency)

        # Extract Cache IDs
        index_ids = [int(result) for result in indices[0]]
        
        # Calculate recall
        correct = len(set(index_ids).intersection(set(groundtruth)))
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
    # Calculate the quartiles
    SSD_lookup = np.percentile(latencies, 90)

    print(f"SSD latency (90th percentile): {SSD_lookup}")


if __name__ == '__main__':
    asyncio.run(main())
