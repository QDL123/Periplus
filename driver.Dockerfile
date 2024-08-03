# Use an official Miniconda image as a parent image
FROM continuumio/miniconda3

# Set the working directory in the container to /app
WORKDIR /app

# Copy the entire working tree into the container at /app
COPY . .

# Change directory to test/e2e
WORKDIR /app/test/e2e

# Install netcat-openbsd
RUN apt-get update && apt-get install -y netcat-openbsd

# Create the conda environment named e2e_env and install dependencies
RUN conda env create -f environment.yml

# Run the script using the conda environment's python interpreter
CMD ["bash", "-c", "source activate e2e_env && python3 e2e_test.py"]
