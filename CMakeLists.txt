# cmake -B build -DCMAKE_BUILD_TYPE=Debug

cmake_minimum_required(VERSION 3.16)
project(SQL_engine CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(sql_core src/tokenize.cpp src/parse.cpp src/load_file.cpp src/operators.cpp src/planner.cpp src/printer.cpp src/helpers.cpp)
target_include_directories(sql_core PUBLIC headers)


add_executable(sql_engine src/main.cpp)
target_link_libraries(sql_engine sql_core)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(unit_tests tests/test_tokenizer.cpp tests/test_parser.cpp tests/test_load_file.cpp tests/test_operators.cpp tests/test_planner.cpp)
target_link_libraries(unit_tests sql_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(unit_tests)