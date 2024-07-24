# Use the official miniconda3 image as the base image
FROM continuumio/miniconda3

# Set the working directory inside the container
WORKDIR /app

# Copy the memory_map_benchmarking.py script into the container
COPY memory_map_benchmarking.py /app/

# Create the datasets directory structure inside the container
RUN mkdir -p /app/datasets/sift

# Copy the contents of the datasets/sift directory into the container
COPY datasets/sift /app/datasets/sift

# Create and activate a conda environment with FAISS installed
RUN conda config --add channels conda-forge && \
    conda create -n faiss_env -y python=3.9 faiss

# Activate the environment and install additional dependencies
RUN echo "source activate faiss_env" > ~/.bashrc
ENV PATH /opt/conda/envs/faiss_env/bin:$PATH

# Make the ./datasets/sift directory a mountable volume
VOLUME /app/datasets/sift

# Set the command to run the memory_map_benchmarking.py script
CMD ["python", "memory_map_benchmarking.py"]
