cmake_minimum_required(VERSION 3.1)
project(PackageTest CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(asio_stream_compressor REQUIRED)

add_executable(test_package example.cpp)
target_link_libraries(test_package asio_stream_compressor::asio_stream_compressor)
