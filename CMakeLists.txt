cmake_minimum_required(VERSION 3.14) # Set the minimum required version of CMake

project(CoreTest) # Name of your project

# Set C++17 as the standard for the project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DASIO_STANDALONE)

# Find the Catch2 package
find_package(Catch2 3 REQUIRED)

# # Find Boost package, specifically looking for the system and thread components which are commonly used with Asio
# find_package(Boost 1.70 REQUIRED COMPONENTS system thread)

# Add the threading package to handle any threading needs of Asio
find_package(Threads REQUIRED)

# List all the source files required for the test executable, including dependencies
set(TEST_SOURCES
    test_core.cpp
    core.cpp
    db_client.cpp
    data.cpp
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

# Define source files for the new executable that uses Boost.Asio
set(ASIO_SOURCES
    driver.cpp
    server.cpp  # Replace with your actual file that uses Boost.Asio
    session.cpp
    cache.cpp
    core.cpp
    db_client.cpp
    args.cpp
    data.cpp
)

# Add another executable that will use Boost.Asio
add_executable(asio_app ${ASIO_SOURCES})

# Specify the directories where the compiler can find headers for the Asio app
target_include_directories(asio_app PRIVATE
    ${Boost_INCLUDE_DIRS}
    /opt/homebrew/opt/faiss/include
    /opt/homebrew/opt/libomp/include
    /opt/homebrew/opt/asio/include
)

# Link libraries to the new executable
target_link_libraries(asio_app PRIVATE
    ${Boost_LIBRARIES}
    /opt/homebrew/opt/faiss/lib/libfaiss.dylib
    /opt/homebrew/opt/libomp/lib/libomp.dylib
)
