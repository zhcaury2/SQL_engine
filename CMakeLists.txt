# cmake -B build -DCMAKE_BUILD_TYPE=Debug

cmake_minimum_required(VERSION 3.16)
project(SQL_engine CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# All real code goes in this library
add_library(sql_core src/tokenize.cpp src/parse.cpp src/load_file.cpp)
target_include_directories(sql_core PUBLIC headers)

# The program itself
add_executable(sql_engine src/main.cpp)
target_link_libraries(sql_engine sql_core)

# GoogleTest (downloaded automatically)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# Tests
enable_testing()
add_executable(test_tokenizer tests/test_tokenizer.cpp tests/test_parser.cpp tests/test_load_file.cpp)
target_link_libraries(test_tokenizer sql_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(test_tokenizer)