cmake_minimum_required(VERSION 3.14) # Set the minimum required version of CMake

project(CacheTest) # Name of your project

# Set C++17 as the standard for the project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the Catch2 package
find_package(Catch2 3 REQUIRED)

# List all the source files required for the test executable, including dependencies
set(TEST_SOURCES
    test_cache.cpp
    cache.cpp
    db_client.cpp
)

# Add an executable for the tests
add_executable(tests ${TEST_SOURCES})

# Specify the directories where the compiler can find headers
target_include_directories(tests PRIVATE
    /opt/homebrew/opt/faiss/include
    /opt/homebrew/opt/libomp/include
)

# Specify the paths where the linker can find libraries
link_directories(
    /opt/homebrew/opt/faiss/lib
    /opt/homebrew/opt/libomp/lib
)

# Link libraries to your executable target
# target_link_libraries(tests PRIVATE Catch2::Catch2WithMain faiss omp)
target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
    /opt/homebrew/opt/faiss/lib/libfaiss.dylib
    /opt/homebrew/opt/libomp/lib/libomp.dylib
)
