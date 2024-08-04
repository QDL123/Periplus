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

# Detect platform and set paths for dependencies
if (APPLE)
    set(DEPENDENCIES_PREFIX "/opt/homebrew/opt")
else()
    set(DEPENDENCIES_PREFIX "/home/linuxbrew/.linuxbrew/opt")
endif()

set(FAISS_INCLUDE_DIR "${DEPENDENCIES_PREFIX}/faiss/include")
set(FAISS_LIBRARY "${DEPENDENCIES_PREFIX}/faiss/lib/libfaiss.dylib")
set(LIBOMP_INCLUDE_DIR "${DEPENDENCIES_PREFIX}/libomp/include")
set(LIBOMP_LIBRARY "${DEPENDENCIES_PREFIX}/libomp/lib/libomp.dylib")
set(CURL_INCLUDE_DIR "${DEPENDENCIES_PREFIX}/curl/include")
set(CURL_LIBRARY "${DEPENDENCIES_PREFIX}/curl/lib/libcurl.dylib")
set(CPR_INCLUDE_DIR "${DEPENDENCIES_PREFIX}/cpr/include")
set(CPR_LIBRARY "${DEPENDENCIES_PREFIX}/cpr/lib/libcpr.dylib")
set(RAPIDJSON_INCLUDE_DIR "${DEPENDENCIES_PREFIX}/rapidjson/include")
set(ASIO_INCLUDE_DIR "${DEPENDENCIES_PREFIX}/asio/include")

# Find the Catch2 package
find_package(Catch2 3 REQUIRED)

# Source files
set(TEST_SOURCES
    test/unit/test_core.cpp
    src/core.cpp
    src/db_client.cpp
    src/data.cpp
)

set(PERIPLUS_SOURCES
    src/driver.cpp
    src/server.cpp
    src/session.cpp
    src/cache.cpp
    src/core.cpp
    src/db_client.cpp
    src/args.cpp
    src/data.cpp
)

# Add an executable for the tests
add_executable(tests ${TEST_SOURCES})
target_include_directories(tests PRIVATE
    ${FAISS_INCLUDE_DIR}
    ${LIBOMP_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
    ${CPR_INCLUDE_DIR}
    ${RAPIDJSON_INCLUDE_DIR}
    ${ASIO_INCLUDE_DIR}
)
target_link_libraries(tests PRIVATE
    ${FAISS_LIBRARY}
    ${LIBOMP_LIBRARY}
    ${CURL_LIBRARY}
    ${CPR_LIBRARY}
    Catch2::Catch2WithMain
)

# Add an executable for Periplus
add_executable(periplus ${PERIPLUS_SOURCES})
target_include_directories(periplus PRIVATE
    ${FAISS_INCLUDE_DIR}
    ${LIBOMP_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
    ${CPR_INCLUDE_DIR}
    ${RAPIDJSON_INCLUDE_DIR}
    ${ASIO_INCLUDE_DIR}
)
target_link_libraries(periplus PRIVATE
    ${FAISS_LIBRARY}
    ${LIBOMP_LIBRARY}
    ${CURL_LIBRARY}
    ${CPR_LIBRARY}
)

# Add AddressSanitizer flags for periplus
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(periplus PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(periplus PRIVATE -fsanitize=address)
endif()
