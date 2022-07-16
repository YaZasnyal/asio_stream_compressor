include(CMakeFindDependencyMacro)
find_dependency(fmt)

include("${CMAKE_CURRENT_LIST_DIR}/asio_stream_compressorTargets.cmake")
