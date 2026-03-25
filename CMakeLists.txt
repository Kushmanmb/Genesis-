cmake_minimum_required(VERSION 3.10)
project(Genesis VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(genesis
    src/main.cpp
    src/Block.cpp
)

target_include_directories(genesis PRIVATE src)
