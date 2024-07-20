cmake_minimum_required(VERSION 3.14)

project(CoreTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DASIO_STANDALONE)

# Suppress deprecated declarations warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-deprecated-declarations)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/wd4996)
endif()

# Find the Catch2 package
find_package(Catch2 3 REQUIRED)

# Add the threading package to handle any threading needs of Asio
find_package(Threads REQUIRED)

# Find libcurl package
find_package(CURL REQUIRED)

# Find the CPR package
find_package(cpr REQUIRED)

# Set the path to the RapidJSON headers manually
set(RAPIDJSON_INCLUDE_DIR "/opt/homebrew/include")

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
    /opt/homebrew/opt/curlpp/include
    /opt/homebrew/opt/cpr/include
    ${CURL_INCLUDE_DIRS}
    ${RAPIDJSON_INCLUDE_DIR}
)

# Specify the paths where the linker can find libraries
link_directories(
    /opt/homebrew/opt/faiss/lib
    /opt/homebrew/opt/libomp/lib
    /opt/homebrew/opt/curlpp/lib
    /opt/homebrew/opt/cpr/lib
)

# Add AddressSanitizer flags
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tests PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(tests PRIVATE -fsanitize=address)
endif()

# Link libraries to your executable target
target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
    /opt/homebrew/opt/faiss/lib/libfaiss.dylib
    /opt/homebrew/opt/libomp/lib/libomp.dylib
    /opt/homebrew/opt/curlpp/lib/libcurlpp.dylib
    ${CURL_LIBRARIES}
    /opt/homebrew/opt/cpr/lib/libcpr.dylib
)

# Define source files for the new executable that uses Asio
set(ASIO_SOURCES
    driver.cpp
    server.cpp  # Replace with your actual file that uses Asio
    session.cpp
    cache.cpp
    core.cpp
    db_client.cpp
    args.cpp
    data.cpp
)

# Add another executable that will use Asio
add_executable(asio_app ${ASIO_SOURCES})

# Specify the directories where the compiler can find headers for the Asio app
target_include_directories(asio_app PRIVATE
    /opt/homebrew/opt/faiss/include
    /opt/homebrew/opt/libomp/include
    /opt/homebrew/opt/asio/include
    /opt/homebrew/opt/curlpp/include
    /opt/homebrew/opt/cpr/include
    ${CURL_INCLUDE_DIRS}
    ${RAPIDJSON_INCLUDE_DIR}
)

# Add AddressSanitizer flags for asio_app
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(asio_app PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(asio_app PRIVATE -fsanitize=address)
endif()

# Link libraries to the new executable
target_link_libraries(asio_app PRIVATE
    /opt/homebrew/opt/faiss/lib/libfaiss.dylib
    /opt/homebrew/opt/libomp/lib/libomp.dylib
    /opt/homebrew/opt/curlpp/lib/libcurlpp.dylib
    /opt/homebrew/opt/cpr/lib/libcpr.dylib
    ${CURL_LIBRARIES}
)