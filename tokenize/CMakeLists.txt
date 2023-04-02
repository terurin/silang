cmake_minimum_required(VERSION 3.0.0)

add_library(tokenize STATIC
  parsers.cpp
  readers.cpp
  tokens.cpp
)
