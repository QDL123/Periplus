# Use an official Python runtime as a parent image
FROM python:3.11-slim

# Set the working directory in the container to /app
WORKDIR /app

# Copy the entire working tree into the container at /app
COPY . .

# Change directory to test/e2e
WORKDIR /app/test/e2e

RUN pip install -r proxy_requirements.txt

# Expose port 8000
EXPOSE 8000

# Run the script using the virtual environment's python interpreter
CMD ["python3", "test_proxy.py"]
