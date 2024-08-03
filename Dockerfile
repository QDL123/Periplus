# Use the official Homebrew Docker image and specify the platform
FROM --platform=linux/amd64 homebrew/brew:latest

# Install dependencies using Homebrew, including cmake
RUN brew install \
    cmake \
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

# Set environment variables for CMake to find Homebrew-installed packages
ENV CMAKE_PREFIX_PATH=/home/linuxbrew/.linuxbrew
ENV CMAKE_INCLUDE_PATH=/home/linuxbrew/.linuxbrew/include
ENV CMAKE_LIBRARY_PATH=/home/linuxbrew/.linuxbrew/lib

# Build the project
RUN cmake -S . -B build \
    && cmake --build build

# Default command
CMD ["./build/periplus", "-p", "3000"]
