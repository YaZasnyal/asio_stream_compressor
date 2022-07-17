cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    asio_stream_compressor
    VERSION 0.1.0
    DESCRIPTION "Wrappers that add ZSTD compression support for asio async streams"
    HOMEPAGE_URL "https://github.com/YaZasnyal/asio_stream_compressor"
    LANGUAGES NONE
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

option(
    asio_stream_compressor_ASIO_STANDALONE
    "Use standalone asio library instead of boost"
    OFF
)
option(
    asio_stream_compressor_ZSTD_SHARED
    "Use shared zstd variand instead of static"
    OFF
)

# ---- Declare library ----

add_library(asio_stream_compressor_asio_stream_compressor INTERFACE)
add_library(asio_stream_compressor::asio_stream_compressor ALIAS asio_stream_compressor_asio_stream_compressor)

set_property(
    TARGET asio_stream_compressor_asio_stream_compressor PROPERTY
    EXPORT_NAME asio_stream_compressor
)

target_include_directories(
    asio_stream_compressor_asio_stream_compressor ${warning_guard}
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_sources(asio_stream_compressor_asio_stream_compressor PRIVATE
    include/asio_stream_compressor/detail/defines.h
    include/asio_stream_compressor/detail/zstd_error_condition.h
    include/asio_stream_compressor/detail/compressor_statistics.h
    include/asio_stream_compressor/detail/read_operation.h
    include/asio_stream_compressor/detail/write_operation.h
    include/asio_stream_compressor/detail/compression_core.h
    include/asio_stream_compressor/asio_stream_compressor.h
    include/asio_stream_compressor/errors.h
    include/asio_stream_compressor/statistics.h
)

target_compile_features(asio_stream_compressor_asio_stream_compressor INTERFACE cxx_std_14)

# Find asio
if (NOT asio_stream_compressor_ASIO_STANDALONE)
    find_package(Boost 1.68.0 REQUIRED)
else()
    find_package(asio REQUIRED)
    target_compile_definitions(asio_stream_compressor_asio_stream_compressor INTERFACE
        ASIO_STEREAM_COMPRESSOR_FLAVOUR_STANDALONE
    )
endif()

# find zstd
find_package(zstd 1.4.0 REQUIRED)
if (NOT TARGET zstd::libzstd_static)
    set(asio_stream_compressor_ZSTD_SHARED ON)
endif()

target_link_libraries(asio_stream_compressor_asio_stream_compressor INTERFACE
    $<BUILD_INTERFACE:$<$<NOT:$<BOOL:${asio_stream_compressor_ASIO_STANDALONE}>>:Boost::boost>>
    $<BUILD_INTERFACE:$<$<BOOL:${asio_stream_compressor_ASIO_STANDALONE}>:asio::asio>>
    $<BUILD_INTERFACE:$<$<NOT:$<BOOL:${asio_stream_compressor_ZSTD_SHARED}>>:zstd::libzstd_static>>
    $<BUILD_INTERFACE:$<$<BOOL:${asio_stream_compressor_ZSTD_SHARED}>:zstd::libzstd_shared>>
)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${asio_stream_compressor_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT asio_stream_compressor_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of asio_stream_compressor"
  )
endif()

include(cmake/dev-mode.cmake)
