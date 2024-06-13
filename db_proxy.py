from fastapi import FastAPI, Query
from pydantic import BaseModel
from typing import List
import faiss
import uuid
import random
import numpy as np

app = FastAPI()

# Initialize FAISS index and storage
dimension = 2  # Example dimension, change as needed
index = faiss.IndexFlatL2(dimension)
stored_objects = []

class StoredObject(BaseModel):
    embedding: List[float]
    document: str
    id: str
    metadata: str

class QueryResult(BaseModel):
    results: List[StoredObject]

@app.get("/v1/load_cell", response_model=QueryResult)
def load_cell(n: int, xq: List[float] = Query(...)):
    if len(xq) != dimension:
        return {"error": f"Query vector must be of dimension {dimension}"}

    # Convert the query vector to a numpy array and reshape for FAISS
    query_vector = np.array(xq, dtype='float32').reshape(1, -1)
    
    # Perform the search in FAISS
    distances, indices = index.search(query_vector, n)
    
    # Gather the results
    results = []
    for idx in indices[0]:
        if idx < len(stored_objects):
            results.append(stored_objects[idx])
    
    return {"results": results}

# Example endpoint to add embeddings to the index
@app.post("/add_embedding")
def add_embedding(embedding: List[float], document: str, id: str, metadata: str):
    if len(embedding) != dimension:
        return {"error": f"Embedding must be of dimension {dimension}"}
    
    # Add the embedding to the FAISS index
    embedding_np = np.array(embedding, dtype='float32').reshape(1, -1)
    index.add(embedding_np)
    
    # Store the object
    stored_object = StoredObject(embedding=embedding, document=document, id=id, metadata=metadata)
    stored_objects.append(stored_object)
    
    return {"status": "success"}


def load_datastore():
    print("Loading the datastore")
    random.seed(42)
    for i in range (250000):
        if (i % 1000 == 0):
            print("Loading the " + str(i) + "th document")

        id = str(uuid.uuid4())
        document = "document: " + str(i)
        metadata = "{'index': " + str(i) + "}"
        embedding = []
        for _ in range(dimension):
            num = random.uniform(-100, 100)
            if (i % 25000 == 0):
                print(str(i) + "th vector starts with " + str(num))
            embedding.append(num)


         # Add the embedding to the FAISS index
        embedding_np = np.array(embedding, dtype='float32').reshape(1, -1)
        index.add(embedding_np)
        object = StoredObject(embedding=embedding, document=document, id=id, metadata=metadata)
        stored_objects.append(object)

    print("FINISHED LOADING!")



# Run the FastAPI app
if __name__ == "__main__":
    import uvicorn
    load_datastore()
    uvicorn.run(app, host="0.0.0.0", port=8000)
