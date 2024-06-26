from fastapi import FastAPI, Query
from pydantic import BaseModel
from typing import List
import faiss
import uuid
import random
import numpy as np
import matplotlib.pyplot as plt


app = FastAPI()

# Initialize FAISS index and storage
dimension = 500  # Example dimension, change as needed
index = faiss.IndexFlatL2(dimension)
stored_objects = []


# Sample IVFFLAT Index
quantizer = faiss.IndexFlatL2(dimension)
numCells = 2000
ivf = faiss.IndexIVFFlat(quantizer, dimension, numCells)

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




def load_datastore():
    print("Loading the datastore")
    random.seed(42)
    num_training = 160000
    embeddings = []
    for i in range (250000):
        if (i % 1000 == 0):
            print("Loading the " + str(i) + "th document")

        id = str(uuid.uuid4())
        document = "document: " + str(i)
        metadata = "{'index': " + str(i) + "}"
        embedding = []
        for _ in range(dimension):
            num = random.uniform(-100, 100)
            # if (i % 25000 == 0):
            #     print(str(i) + "th vector starts with " + str(num))
            embedding.append(num)

        embeddings.append(embedding)

         # Add the embedding to the FAISS index
            
        # if i == num_training:
        #     ivf.train(np.vstack(embeddings)) 

        object = StoredObject(embedding=embedding, document=document, id=id, metadata=metadata)
        stored_objects.append(object)
    
    ivf.train(np.vstack(embeddings))
    print("Adding embeddings to IVF index")
    ivf.add(np.vstack(embeddings))
    index.add(np.vstack(embeddings))

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
        
    
    ivf.nprobe = 1


    # for i in range(len(embeddings)):
    #     # Getting density of vectors in the cell
    #     xq = np.array(embeddings[i], dtype='float32').reshape(1, -1)
    #     nn_distances, nn_indices = index.search(xq, 10000)
    #     _, centroid_indices = quantizer.search(xq, 1)
    #     print("centroid_indices len: " + str(len(centroid_indices)))
    #     centroid = centroid_indices[0][0]
    #     print("centroid: " + str(centroid))
    #     cell_size = ivf.invlists.list_size(int(centroid))
    #     print("cell size: " + str(cell_size))
    #     ivf_distances, ivf_indices = ivf.search(xq, cell_size)

    #     # Loop through nn to 
    #     num_hits = 0
    #     rolling_densities = []
    #     cell_percentages = []
    #     for j in range(len(nn_indices[0])):
    #         # Detect if this embedding belongs to the relevant cell
    #         xq_centroid_nn = np.array(embeddings[nn_indices[0][j]], dtype='float32').reshape(1, -1)
    #         _, nearest_centroid = quantizer.search(xq_centroid_nn, 1)
    #         print("nearest centroid: " + str(nearest_centroid[0][0]))
    #         if nearest_centroid[0][0] == centroid:
    #             # This vector belongs to the cell we are trying to load
    #             num_hits += 1
    #             print("Found hit at j: " + str(j))
    #         density = num_hits / (j + 1)
    #         if density > 0:
    #             print("density > 0")

    #         rolling_densities.append(density)
    #         percent_of_cell = num_hits / cell_size
    #         cell_percentages.append(percent_of_cell)
    #         vectors_queried = j

    #     print("length of densities: " + str(len(rolling_densities)))
    #     print("length of percentages: " + str(len(cell_percentages)))
    #     for j in range(100):
    #         print("density: " + str(rolling_densities[j]) + ", percent loaded: " + str(cell_percentages[j]))
    #     plot_density_versus_cell_percentage(np.array(rolling_densities), np.array(cell_percentages))


        

    print("FINISHED LOADING!")



# Run the FastAPI app
if __name__ == "__main__":
    import uvicorn
    load_datastore()
    uvicorn.run(app, host="0.0.0.0", port=8000)
