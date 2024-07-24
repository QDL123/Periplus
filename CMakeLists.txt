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
set(RAPIDJSON_INCLUDE_DIR "/home/linuxbrew/.linuxbrew/include")

# List all the source files required for the test executable, including dependencies
set(TEST_SOURCES
    test/unit/test_core.cpp
    src/core.cpp
    src/db_client.cpp
    src/data.cpp
)

# Add an executable for the tests
add_executable(tests ${TEST_SOURCES})

# Specify the directories where the compiler can find headers
target_include_directories(tests PRIVATE
    /home/linuxbrew/.linuxbrew/opt/faiss/include
    /home/linuxbrew/.linuxbrew/opt/libomp/include
    /home/linuxbrew/.linuxbrew/opt/curlpp/include
    /home/linuxbrew/.linuxbrew/opt/cpr/include
    ${CURL_INCLUDE_DIRS}
    ${RAPIDJSON_INCLUDE_DIR}
)

# Specify the paths where the linker can find libraries
link_directories(
    /home/linuxbrew/.linuxbrew/opt/faiss/lib
    /home/linuxbrew/.linuxbrew/opt/libomp/lib
    /home/linuxbrew/.linuxbrew/opt/curlpp/lib
    /home/linuxbrew/.linuxbrew/opt/cpr/lib
)

# Add AddressSanitizer flags
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(tests PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(tests PRIVATE -fsanitize=address)
endif()

# Link libraries to your executable target
target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
    /home/linuxbrew/.linuxbrew/opt/faiss/lib/libfaiss.so
    /home/linuxbrew/.linuxbrew/opt/libomp/lib/libomp.so
    /home/linuxbrew/.linuxbrew/opt/curlpp/lib/libcurlpp.so
    ${CURL_LIBRARIES}
    /home/linuxbrew/.linuxbrew/opt/cpr/lib/libcpr.so
)

# Define source files for the new executable that uses Asio
set(ASIO_SOURCES
    src/driver.cpp
    src/server.cpp  # Replace with your actual file that uses Asio
    src/session.cpp
    src/cache.cpp
    src/core.cpp
    src/db_client.cpp
    src/args.cpp
    src/data.cpp
)

# Add another executable that will use Asio
add_executable(asio_app ${ASIO_SOURCES})

# Specify the directories where the compiler can find headers for the Asio app
target_include_directories(asio_app PRIVATE
    /home/linuxbrew/.linuxbrew/opt/faiss/include
    /home/linuxbrew/.linuxbrew/opt/libomp/include
    /home/linuxbrew/.linuxbrew/opt/asio/include
    /home/linuxbrew/.linuxbrew/opt/curlpp/include
    /home/linuxbrew/.linuxbrew/opt/cpr/include
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
    /home/linuxbrew/.linuxbrew/opt/faiss/lib/libfaiss.so
    /home/linuxbrew/.linuxbrew/opt/libomp/lib/libomp.so
    /home/linuxbrew/.linuxbrew/opt/curlpp/lib/libcurlpp.so
    ${CURL_LIBRARIES}
    /home/linuxbrew/.linuxbrew/opt/cpr/lib/libcpr.so
)
