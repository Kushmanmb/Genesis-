cmake_minimum_required(VERSION 3.10)
project(Genesis VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(genesis
    src/main.cpp
    src/sha256.cpp
    src/Block.cpp
    src/Blockchain.cpp
    src/Node.cpp
)

target_include_directories(genesis PRIVATE src)

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

include(GoogleTest)
gtest_discover_tests(genesis_tests)
