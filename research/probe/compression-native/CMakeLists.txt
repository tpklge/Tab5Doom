cmake_minimum_required(VERSION 3.16)
project(ChexCompressionRegression C)
set(CMAKE_C_STANDARD 99)
get_filename_component(ZD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../zdoom-2.8.1" ABSOLUTE)
include("${CMAKE_CURRENT_LIST_DIR}/../../compression_sources.cmake")
add_executable(compression_test ${ZD_COMPRESSION_SOURCES} "../compression_probe.c")
target_include_directories(compression_test PRIVATE "${ZD_ROOT}/zlib"
    "${ZD_ROOT}/bzip2" "${ZD_ROOT}/lzma/C")
target_compile_definitions(compression_test PRIVATE BZ_NO_STDIO _7ZIP_ST
    _7ZIP_PPMD_SUPPPORT CHEX_COMPRESSION_HOST_TEST)
# zlib 1.2.7's legacy Mac fdopen fallback must follow system declarations.
target_compile_options(compression_test PRIVATE -include stdio.h)
enable_testing()
add_test(NAME decompression_and_corruption COMMAND compression_test)
