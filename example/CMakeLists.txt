cmake_minimum_required(VERSION 3.0)
include_directories(.)
add_definitions("-std=c++2a")

add_executable(example example.cpp ../linenoise/model/RawReader.hpp ../linenoise/model/exceptions/IOException.hpp)
