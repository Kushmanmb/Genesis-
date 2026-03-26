cmake_minimum_required(VERSION 3.10)
project(Genesis VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable debug logging by passing -DGENESIS_DEBUG=ON to cmake.
option(GENESIS_DEBUG "Enable debug-level logging to stderr" OFF)

add_executable(genesis
    src/main.cpp
    src/sha256.cpp
    src/Block.cpp
    src/Blockchain.cpp
    src/Node.cpp
)

target_include_directories(genesis PRIVATE src)

if(GENESIS_DEBUG)
    target_compile_definitions(genesis PRIVATE GENESIS_DEBUG)
endif()

# Tests
enable_testing()

find_package(GTest REQUIRED)

add_executable(genesis_tests
    tests/test_blockchain.cpp
    src/sha256.cpp
    src/Block.cpp
    src/Blockchain.cpp
    src/Node.cpp
)

target_include_directories(genesis_tests PRIVATE src)
target_link_libraries(genesis_tests GTest::GTest GTest::Main)

if(GENESIS_DEBUG)
    target_compile_definitions(genesis_tests PRIVATE GENESIS_DEBUG)
endif()

include(GoogleTest)
gtest_discover_tests(genesis_tests)
