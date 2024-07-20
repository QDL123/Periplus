from fastapi import FastAPI, Query
from pydantic import BaseModel
from typing import List
import faiss
import uuid
import random
import numpy as np
import matplotlib.pyplot as plt
from pinecone import Pinecone, ServerlessSpec


app = FastAPI()

# Initialize FAISS index and storage
dimension = 128  # Example dimension, change as needed
index = faiss.IndexFlatL2(dimension)
stored_objects = []
object_indices = {}


# Sample IVFFLAT Index
quantizer = faiss.IndexFlatL2(dimension)
numCells = 2000

class StoredObject(BaseModel):
    embedding: List[float]
    document: str
    id: str
    metadata: str

class QueryResult(BaseModel):
    results: List[StoredObject]

class IdsModel(BaseModel):
    ids: List[str]

# pc = Pinecone(
#         api_key='95c5de12-ae3b-43dd-b401-baee878d4c71'
#     )
# index_name = 'sift1m-index-2'
# index = pc.Index(index_name)
# print("Connected to index")

@app.get("/v1/load_cell", response_model=QueryResult)
def load_cell(n: int, xq: List[float] = Query(...)):
    if len(xq) != dimension:
        return {"error": f"Query vector must be of dimension {dimension}"}
    
    print("NUMBER OF REQUESTED VECTORS: " + str(n))

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


@app.post("/v1/load_cell")
async def handle_post(request: IdsModel):
    # Extract the list of IDs from the request
    ids = request.ids
    
    # Process the IDs as needed
    # For example, just print them here
    results = []
    for id in ids:
        results.append(stored_objects[object_indices[id]])

    # Return a response
    return {"results": results}


def split_list(lst, chunk_size=100):
    return [lst[i:i + chunk_size] for i in range(0, len(lst), chunk_size)]

@app.post("/v1/pinecone")
async def handle_pinecone_load(request: IdsModel):
    print("Loading data from pinecone")
    ids = request.ids
    print("Received request with first id: " + str(ids[0]))

    results = []
    id_lists = split_list(ids)
    for ids in id_lists:
        response = index.fetch(ids=ids)
        vectors = response['vectors']
        for id, data in vectors.items():
            embedding = data['values']
            if len(embedding) == 0:
                print("ERROR: FOUND EMPTY EMBEDDING")
            results.append(StoredObject(embedding=embedding, document="document", id=id, metadata="{}"))

    return { 'results': results }

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


def plot_distances_from_centroid(ivf_distances):
    # Generate a simple line graph
    num_distances = len(ivf_distances.flatten())
    print("num_distances: " + str(num_distances))
    x_values = range(num_distances)

    plt.plot(x_values, ivf_distances.flatten(), marker='o')
    plt.title('Distance from centroid of embeddings in cell')
    plt.xlabel('Xth Vector')
    plt.ylabel('Distance')

    # Display the graph
    plt.show()


def plot_density_versus_cell_percentage(densities, percent_of_cell):
    # Generate a scatter plot
    x = np.random.rand(10000)  # Generate 10,000 random x values
    y = np.random.rand(10000)  # Generate 10,000 random y values

    # Generate a scatter plot
    # plt.scatter(x, y)
    plt.scatter(densities, percent_of_cell, marker='o')
    plt.title('Scatter plot of density of vectors in cell versus cell percentage loaded (D=500)')
    plt.xlabel('Density of vectors belonging to the cell of interest')
    plt.ylabel('Percentage of total cell queried')

    # Display the scatter plot
    plt.show()


def generate_plots(embeddings, index, ivf):
    cell_counts = []
    for i in range(numCells):
        cell_counts.append(ivf.invlists.list_size(i))
    
    centroids = quantizer.reconstruct_n(0, numCells)
    print("centroids length: " + str(len(centroids)))

    for i in range(len(centroids)):
        # Retrieve all the nearest neighbors, including other cells, to the centroid
        centroid = np.array([centroids[i]])
        cell_size = ivf.invlists.list_size(i)
        # print("centroid:")
        # print(centroid)
        _, nn_indices = index.search(centroid, 10000)

        print("nn_indices: ")
        print(nn_indices)

        # Go through the nearest neighbors and check which belong to the cell defined by 
        # centroid, and use that to compute the rolling densities, and percent of cell 
        # queried
        rolling_densities = []
        cell_percentages = []
        num_hits = 0
        for j in range(len(nn_indices[0])):
            # Get the centroid index of the jth nearest neighbor to centroid
            nn_xq = np.array([embeddings[nn_indices[0][j]]])
            _, nn_centroid = quantizer.search(nn_xq, 1)

            # Check if the nn_centroid matches the cell we are attempting to load
            # print("nn_centroid: " + str(nn_centroid[0][0]))
            # print("centroid we're attempting to load: " + str(i))
            if nn_centroid[0][0] == i:
                num_hits += 1

            density = num_hits / (j + 1)

            rolling_densities.append(density)
            print("j: " + str(j) + "density: " + str(density))
            percent_of_cell = num_hits / cell_size
            print("j: " + str(j) + "percent of cell loaded: " + str(percent_of_cell))
            cell_percentages.append(percent_of_cell)
        plot_density_versus_cell_percentage(rolling_densities, cell_percentages)




def load_datastore():
    print("Loading the datastore")
    random.seed(42)
    embeddings = []
    for i in range (50000):
        document = "document: " + str(i)
        namespace = uuid.UUID('12345678-1234-5678-1234-567812345678')

        # Generate UUID3 (MD5 hash based)
        uuid3 = uuid.uuid3(namespace, document)
        # id = str(uuid.uuid4())
        id = str(uuid3)
        if (i % 1000 == 0):
            print("Loading the " + str(i) + "th document")
            print("id: " + id)

        metadata = "{'index': " + str(i) + "}"
        embedding = []
        for _ in range(dimension):
            num = random.uniform(-100, 100)
            embedding.append(num)

        embeddings.append(embedding)

        object = StoredObject(embedding=embedding, document=document, id=id, metadata=metadata)
        object_indices[id] = len(stored_objects)
        stored_objects.append(object)
    
    print("Index type:")
    print(type(index))
    index.add(np.vstack(embeddings))

        

    print("FINISHED LOADING!")



# Run the FastAPI app
if __name__ == "__main__":
    import uvicorn
    load_datastore()
    uvicorn.run(app, host="0.0.0.0", port=8000)
