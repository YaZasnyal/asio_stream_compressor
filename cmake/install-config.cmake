include(CMakeFindDependencyMacro)

# Options

option(
    asio_stream_compressor_ASIO_STANDALONE
    "Use standalone asio library instead of boost"
    OFF
)

# Dependencies

if (NOT TARGET zstd::libzstd_static)
    find_dependency(zstd 1.4.0 REQUIRED)
endif()

# Targets

include("${CMAKE_CURRENT_LIST_DIR}/asio_stream_compressorTargets.cmake")

if (NOT asio_stream_compressor_ASIO_STANDALONE)
    if (NOT TARGET Boost::boost)
        find_dependency(Boost 1.68.0 REQUIRED)
    endif()
    target_link_libraries(asio_stream_compressor::asio_stream_compressor INTERFACE Boost::boost)
else()
    if (NOT TARGET asio::asio)
        find_dependency(asio REQUIRED)
    endif()
    target_compile_definitions(asio_stream_compressor::asio_stream_compressor INTERFACE
        ASIO_STEREAM_COMPRESSOR_FLAVOUR_STANDALONE
    )
    target_link_libraries(asio_stream_compressor::asio_stream_compressor INTERFACE asio::asio)
endif()
