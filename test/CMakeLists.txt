cmake_minimum_required(VERSION 3.14)

project(asio_stream_compressorTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(asio_stream_compressor REQUIRED)
  enable_testing()
endif()

find_package(GTest REQUIRED)

# ---- Tests ----

add_executable(asio_stream_compressor_test)
target_sources(asio_stream_compressor_test PRIVATE
  source/asio_stream_compressor_test.cpp
  source/asio_stream_compressor_api_test.cpp
)
target_link_libraries(asio_stream_compressor_test PRIVATE
  asio_stream_compressor::asio_stream_compressor
  Boost::coroutine
  GTest::gtest_main
)
target_compile_features(asio_stream_compressor_test PRIVATE cxx_std_17)
add_test(NAME asio_stream_compressor_test COMMAND asio_stream_compressor_test)

# ---- End-of-file commands ----

add_folders(Test)
