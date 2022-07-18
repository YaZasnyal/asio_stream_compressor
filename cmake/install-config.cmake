include(CMakeFindDependencyMacro)

# Options

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

# Targets

include("${CMAKE_CURRENT_LIST_DIR}/asio_stream_compressorTargets.cmake")

# Dependencies

if (NOT asio_stream_compressor_ASIO_STANDALONE)
    find_dependency(Boost 1.70.0 REQUIRED)
    target_link_libraries(asio_stream_compressor::asio_stream_compressor INTERFACE Boost::boost)
else()
    find_dependency(asio REQUIRED)
    target_compile_definitions(asio_stream_compressor::asio_stream_compressor INTERFACE
        ASIO_STEREAM_COMPRESSOR_FLAVOUR_STANDALONE
    )
    target_link_libraries(asio_stream_compressor::asio_stream_compressor INTERFACE asio::asio)
endif()

find_dependency(zstd 1.4.0 REQUIRED)
if (asio_stream_compressor_ZSTD_SHARED)
    target_link_libraries(asio_stream_compressor::asio_stream_compressor INTERFACE zstd::libzstd_shared)
else()
    target_link_libraries(asio_stream_compressor::asio_stream_compressor INTERFACE zstd::libzstd_static)
endif()
