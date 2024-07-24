from periplus_proxy import ProxyController, IdsModel, QueryResult, StoredObject
from pinecone import Pinecone


pc = Pinecone(
        api_key='<PINECONE-API-KEY>'
    )
index_name = '<PINECONE-INDEX-NAME>'
index = pc.Index(index_name)
print("Connected to index")


def split_list(lst, chunk_size=100):
    return [lst[i:i + chunk_size] for i in range(0, len(lst), chunk_size)]

async def fetch_ids(request: IdsModel) -> QueryResult:
    print("fetch ids")
    ids = request.ids
    print("Received request with first id: " + str(ids[0]))

    results = []
    id_lists = split_list(ids)
    for ids in id_lists:
        response = index.fetch(ids=ids)
        vectors = response['vectors']
        for id, data in vectors.items():
            embedding = data['values']
            results.append(StoredObject(embedding=embedding, document="document", id=id, metadata="{}"))

    print("len:")
    print(len(results))
    print("results: ")
    print(results)
    return QueryResult(results=results)

# Usage
if __name__ == "__main__":
    endpoint = "/api/v1/load_data"
    controller = ProxyController(endpoint=endpoint, fetch_ids=fetch_ids)
    controller.run()