import asyncio
import random
from python_client import CacheClient



def generate_training_data(d):
    data = []
    random.seed(42)

    for _ in range(160000):
        vector = []
        for _ in range(d):
            vector.append(random.uniform(-100, 100))
        
        data.append(vector)

    return data


async def main():
    print("Starting main")

    client = CacheClient("localhost", 13)

    url = "http://localhost:8000/v1/load_cell"
    d = 2

    await client.initialize(d=d, db_url=url)

    training_data = generate_training_data(d)
    await client.train(training_data)

    await client.load([1, 1])

    res = await client.search(1, [[1, 1]])

    print("QUERY RESPONSE:")
    print(res)


if __name__ == "__main__":
    asyncio.run(main())