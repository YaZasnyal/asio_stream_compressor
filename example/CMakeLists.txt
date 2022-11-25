cmake_minimum_required(VERSION 3.14)

project(asio_stream_compressorExamples CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(asio_stream_compressor REQUIRED)
endif()

add_custom_target(run-examples)

function(add_example NAME)
  add_executable("${NAME}" "${NAME}.cpp")
  target_link_libraries("${NAME}" PRIVATE asio_stream_compressor::asio_stream_compressor Boost::coroutine Boost::context)
  target_include_directories("${NAME}" PRIVATE detail)
  target_compile_features("${NAME}" PRIVATE cxx_std_11)
  add_custom_target("run_${NAME}" COMMAND "${NAME}" VERBATIM)
  add_dependencies("run_${NAME}" "${NAME}")
  add_dependencies(run-examples "run_${NAME}")
endfunction()

add_example(basic_usage)

add_folders(Example)
