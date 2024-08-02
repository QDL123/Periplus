# Use an official Python runtime as a parent image
FROM python:3.11-slim

# Set the working directory in the container to /app
WORKDIR /app

# Copy the entire working tree into the container at /app
COPY . .

# Change directory to test/e2e
WORKDIR /app/test/e2e

# Create a Python virtual environment named e2e_test_env
# RUN python -m venv e2e_test_env

# # Activate the virtual environment and install dependencies
# RUN pip install --upgrade pip

# RUN . e2e_test_env/bin/activate

RUN pip install -r proxy_requirements.txt

# Expose port 8000
EXPOSE 8000

# Run the script using the virtual environment's python interpreter
CMD ["python3", "test_proxy.py"]
