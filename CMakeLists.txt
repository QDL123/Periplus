cmake_minimum_required(VERSION 3.14)

project(Periplus)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DASIO_STANDALONE)

# Suppress deprecated declarations warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-deprecated-declarations)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/wd4996)
endif()

# Enforce consistent floating-point behavior
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -ffloat-store
        -frounding-math
        -mfpmath=sse
    )
endif()

# Find the Catch2 package
find_package(Catch2 3 REQUIRED)

# Add the threading package to handle any threading needs of Asio
find_package(Threads REQUIRED)

# TODO: Can we get rid of that?
# Find libcurl package
find_package(CURL REQUIRED)

# Find the CPR package
find_package(cpr REQUIRED)

# Set the path to the RapidJSON headers manually
set(RAPIDJSON_INCLUDE_DIR "/home/linuxbrew/.linuxbrew/include")

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
    /opt/homebrew/opt/faiss/include
    /opt/homebrew/opt/libomp/include
    /opt/homebrew/opt/curlpp/include
    /opt/homebrew/opt/cpr/include
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
set(PERIPLUS_SOURCES
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
add_executable(periplus ${PERIPLUS_SOURCES})

# Specify the directories where the compiler can find headers for the Asio app
target_include_directories(periplus PRIVATE
    /home/linuxbrew/.linuxbrew/opt/faiss/include
    /home/linuxbrew/.linuxbrew/opt/libomp/include
    /home/linuxbrew/.linuxbrew/opt/asio/include
    /home/linuxbrew/.linuxbrew/opt/curlpp/include
    /home/linuxbrew/.linuxbrew/opt/cpr/include
    ${CURL_INCLUDE_DIRS}
    ${RAPIDJSON_INCLUDE_DIR}
)

# Add AddressSanitizer flags for periplus
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(periplus PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(periplus PRIVATE -fsanitize=address)
endif()

# Apply floating-point flags specifically to the periplus target
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(periplus PRIVATE
        -ffloat-store
        -frounding-math
        -mfpmath=sse
    )
endif()

# Link libraries to the new executable
target_link_libraries(periplus PRIVATE
    /home/linuxbrew/.linuxbrew/opt/faiss/lib/libfaiss.so
    /home/linuxbrew/.linuxbrew/opt/libomp/lib/libomp.so
    /home/linuxbrew/.linuxbrew/opt/curlpp/lib/libcurlpp.so
    ${CURL_LIBRARIES}
    /home/linuxbrew/.linuxbrew/opt/cpr/lib/libcpr.so
)
