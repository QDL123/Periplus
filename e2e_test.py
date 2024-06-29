import asyncio
import random
import uuid
import faiss
import math
import numpy as np
from python_client import CacheClient


def generate_ids(num_ids):
    namespace = uuid.UUID('12345678-1234-5678-1234-567812345678')
    ids = []
    for i in range(num_ids):
        document = "document: " + str(i)
        uuid3 = uuid.uuid3(namespace, document)
        id = str(uuid3)
        ids.append(id)

    return ids

def generate_embeddings(d, num_embeddings):
    data = []
    random.seed(42)

    for _ in range(num_embeddings):
        vector = []
        for _ in range(d):
            vector.append(random.uniform(-100, 100))
        
        data.append(vector)

    return data


async def main():
    print("Starting e2e tests")
    num_docs = 50000
    print("generating ids")
    ids = generate_ids(num_docs)

    url = "http://localhost:8000/v1/load_cell"
    d = 500
    numCells = int(4 * math.sqrt(num_docs))

    print("generating embeddings")
    embeddings = generate_embeddings(d, num_docs)

    print("building local index")
    quantizer = faiss.IndexFlatL2(d)
    index = faiss.IndexIVFFlat(quantizer, d, numCells)

    index.train(np.array(embeddings))
    index.add(np.vstack(embeddings))

    client = CacheClient("localhost", 13)

    print("initializing cache")
    await client.initialize(d=d, db_url=url, options={"nTotal":num_docs})
    
    print("training cache")
    await client.train(training_data=embeddings)

    print("adding vectors to cache")
    await client.add(ids=ids, embeddings=embeddings)

    num_correct = 0
    num_error = 0
    for i in range(100):
        print("Testing query number: " + str(i))
        await client.load(embeddings[i])

        k = 5
        res = await client.search(k, [embeddings[i]])
        _, indices = index.search(np.array([embeddings[i]]), k)

        correct_ids = []
        for j in range(len(indices[0])):
            correct_ids.append(ids[indices[0][j]])
        
        cache_ids = []
        for j in range(len(res[0])):
            cache_ids.append(res[0][j][0])
        
        print("Correct IDs:" + str(correct_ids))
        print("Cache IDs: " + str(cache_ids))
        if correct_ids == cache_ids:
            print("Query " + str(i) + " is correct!")
            num_correct += 1
        else:
            print("ERROR: QUERY " + str(i) + " FAILED")
            num_error += 1
        
        print("NUM CORRECT: " + str(num_correct))
        print("NUM WITH ERROR: " + str(num_error))




if __name__ == "__main__":
    asyncio.run(main())