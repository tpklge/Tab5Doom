get_filename_component(ZD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../zdoom-2.8.1" ABSOLUTE)
set(ZD_EXTRA_DECODERS ON)
include("${CMAKE_CURRENT_LIST_DIR}/../../../compression_sources.cmake")
idf_component_register(SRCS ${ZD_COMPRESSION_SOURCES}
    INCLUDE_DIRS "${ZD_ROOT}/zlib" "${ZD_ROOT}/bzip2" "${ZD_ROOT}/lzma/C"
        "${ZD_ROOT}/jpeg-6b" "${ZD_ROOT}/gdtoa" "${ZD_ROOT}/../generated")
target_compile_definitions(${COMPONENT_LIB} PRIVATE BZ_NO_STDIO _7ZIP_ST _7ZIP_PPMD_SUPPPORT INFNAN_CHECK)
# Keep legacy upstream diagnostics visible without making them IDF errors.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error -fsigned-char)
