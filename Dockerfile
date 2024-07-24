# Use the official Homebrew Docker image and specify the platform
FROM --platform=linux/amd64 homebrew/brew:latest

# Install dependencies using Homebrew
RUN brew install \
    catch2 \
    asio \
    curl \
    cpr \
    rapidjson \
    faiss \
    libomp \
    curlpp

# Set working directory
WORKDIR /app

# Copy the CMakeLists.txt file and source files to the container
COPY . .

# Build the project
RUN cmake -S . -B build \
    && cmake --build build


# Default command
CMD ["./build/asio_app", "-p", "3000"]
