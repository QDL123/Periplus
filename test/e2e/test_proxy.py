import random
import uuid

from periplus_proxy import ProxyController, IdsModel, QueryResult, StoredObject


stored_objects = []
object_indices = {}

def load_test_data(d):
    print("Loading test data")
    random.seed(42)
    for i in range (50000):
        document = "document: " + str(i)
        namespace = uuid.UUID('12345678-1234-5678-1234-567812345678')

        uuid3 = uuid.uuid3(namespace, document)
        id = str(uuid3)
        if (i % 1000 == 0):
            print("Loading the " + str(i) + "th document")
            print("id: " + id)

        metadata = "{'index': " + str(i) + "}"
        embedding = []
        for _ in range(d):
            num = random.uniform(-100, 100)
            embedding.append(num)

        object = StoredObject(embedding=embedding, document=document, id=id, metadata=metadata)
        object_indices[id] = len(stored_objects)
        stored_objects.append(object)


async def fetch_ids(request: IdsModel) -> QueryResult:
    print("Fetching ids")
    ids = request.ids
    
    # Process the IDs as needed
    # For example, just print them here
    results = []
    for id in ids:
        results.append(stored_objects[object_indices[id]])
    # Return a response
    return QueryResult(results=results)


# Usage
if __name__ == "__main__":
    d = 128
    load_test_data(d)
    secret = 'your-webhook-secret'
    endpoint = "/api/v1/load_data"
    controller = ProxyController(endpoint=endpoint, fetch_ids=fetch_ids)
    controller.run()