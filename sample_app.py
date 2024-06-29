import asyncio
import random
import uuid
from python_client import CacheClient
from time import perf_counter



def generate_training_data(d, num_embeddings):
    data = []
    random.seed(42)

    for _ in range(num_embeddings):
        vector = []
        for _ in range(d):
            vector.append(random.uniform(-100, 100))
        
        data.append(vector)

    return data

def generate_ids(num_ids):
    namespace = uuid.UUID('12345678-1234-5678-1234-567812345678')
    ids = []
    for i in range(num_ids):
        document = "document: " + str(i)
        uuid3 = uuid.uuid3(namespace, document)
        id = str(uuid3)
        ids.append(id)

    return ids

async def prepare_cache(client):
    url = "http://localhost:8000/v1/load_cell"
    d = 2

    print("Initializing")
    await client.initialize(d=d, db_url=url, options={ "nTotal": 50000 })

    print("Generating Training data")
    training_data = generate_training_data(d=d, num_embeddings=50000)

    print("Training")
    await client.train(training_data)

    print("Adding")
    ids = generate_ids(len(training_data))
    await client.add(ids=ids, embeddings=training_data)

    print("Loading cell associated with vector 10")
    print("Loading cell with centroid beginning with " + str(training_data[10][0]) + " and ending with " + str(training_data[10][1]))
    await client.load(training_data[10])

    print("Loading cell associated with vector 100")
    print("Loading cell with centroid beginning with " + str(training_data[100][0]) + " and ending with " + str(training_data[100][1]))
    await client.load(training_data[100])

    print("Loading cell associated with vector 1000")
    print("Loading cell with centroid beginning with " + str(training_data[1000][0]) + " and ending with " + str(training_data[1000][1]))
    await client.load(training_data[1000])



    return training_data


async def find_cache_hit_prob():

    client = CacheClient("localhost", 13)
    print("initializing")
    await client.initialize(d=2, db_url="http://localhost:8000/v1/load_cell")
    print("generating training data")
    training_data = generate_training_data(500)

    print("training")
    await client.train(training_data=training_data)

    print("beginning tests")
    hits = 0
    partial_hits = 0
    for i in range(1000):
        print("Testing vec " + str(i))
        await client.load(training_data[i])
        res = await client.search(2, [training_data[i]])
        if len(res[0]) == 2:
            hits += 1
            print("hit: numHits is now " + str(hits))
        elif len(res[0]) == 1:
            partial_hits += 1
            print("partial hit")
        else:
            print("miss")
        
        await client.evict(training_data[i])
    
    print("num hits: " + str(hits))
    print("num partial hits: " + str(partial_hits))



async def main():
    print("Starting main")

    client = CacheClient("localhost", 13)

    data = await prepare_cache(client)

    print("Querying")

    while True:
        index = input("Enter index of vector to query with, or exit to quit:")
        if index.lower() == 'exit':
            break

        start_time = perf_counter()
        res = await client.search(2, [data[int(index)]])
        end_time = perf_counter()

        query_time = end_time - start_time

        print(f"Query time: {query_time} seconds")

        print("res length: " + str(len(res[0])))
        print("QUERY RESPONSE:")
        print(res)


if __name__ == "__main__":
    asyncio.run(main())
    # asyncio.run(find_cache_hit_prob())