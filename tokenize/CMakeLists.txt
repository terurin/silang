cmake_minimum_required(VERSION 3.0.0)

add_library(tokenize STATIC
  parsers.cpp
  readers.cpp
  tokens.cpp
)

add_executable(tokenize_readers_tests readers.cpp readers_test.cpp)
add_test(NAME readers_tests COMMAND tokenize_readers_tests)

add_executable(tokenize_parsers_tests readers.cpp parsers.cpp parsers_test.cpp)
add_test(NAME parsers_tests COMMAND tokenize_parsers_tests)

add_executable(tokenize_benchmark readers.cpp parsers.cpp tokens.cpp tokenize_benchmark.cpp)
