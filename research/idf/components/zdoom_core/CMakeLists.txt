get_filename_component(ZD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../zdoom-2.8.1" ABSOLUTE)
get_filename_component(GENERATED "${CMAKE_CURRENT_LIST_DIR}/../../../generated" ABSOLUTE)
get_filename_component(PROBE "${CMAKE_CURRENT_LIST_DIR}/../../../probe" ABSOLUTE)
if(NOT EXISTS "${GENERATED}/sc_man_scanner.h")
    message(FATAL_ERROR "Run python3 tools/prepare_zdoom.py before this build")
endif()

# Compile the core separately from the desktop platform and audio libraries.
# ESP-IDF platform contracts are supplied by zdoom_platform. Keep all units
# so the linker retains actor/action registration entries.
file(READ "${ZD_ROOT}/src/CMakeLists.txt" UPSTREAM_CMAKE)
string(REGEX MATCH "add_executable\\( zdoom[^)]*\\)" UPSTREAM_TARGET "${UPSTREAM_CMAKE}")
string(REGEX MATCHALL "[A-Za-z_0-9/]+\\.(cpp|c)" CORE_NAMES "${UPSTREAM_TARGET}")
set(CORE_SOURCES "${PROBE}/fixed_probe.cpp")
list(APPEND CORE_SOURCES "${ZD_ROOT}/src/sound/i_sound.cpp")
foreach(NAME IN LISTS CORE_NAMES)
    if(NAME MATCHES "^(sound|oplsynth|timidity|wildmidi)/" OR
       NAME MATCHES "^(x86|i_net|m_specialpaths)\\.cpp$")
        continue()
    endif()
    if(EXISTS "${ZD_ROOT}/src/${NAME}")
        list(APPEND CORE_SOURCES "${ZD_ROOT}/src/${NAME}")
    endif()
endforeach()
list(REMOVE_DUPLICATES CORE_SOURCES)

idf_component_register(WHOLE_ARCHIVE SRCS ${CORE_SOURCES}
    INCLUDE_DIRS "${ZD_ROOT}/src" "${GENERATED}" "${ZD_ROOT}/src/posix"
        "${ZD_ROOT}/src/g_doom" "${ZD_ROOT}/src/g_heretic" "${ZD_ROOT}/src/g_hexen"
        "${ZD_ROOT}/src/g_raven" "${ZD_ROOT}/src/g_strife" "${ZD_ROOT}/src/g_shared"
        "${ZD_ROOT}/src/sound" "${ZD_ROOT}/src/textures" "${ZD_ROOT}/src/thingdef"
        "${ZD_ROOT}/src/xlat" "${ZD_ROOT}/src/oplsynth" "${ZD_ROOT}/src/timidity"
        "${ZD_ROOT}/zlib" "${ZD_ROOT}/bzip2" "${ZD_ROOT}/lzma/C"
        "${ZD_ROOT}/gdtoa" "${ZD_ROOT}/dumb/include" "${ZD_ROOT}/jpeg-6b"
    REQUIRES zdoom_compression
    LDFRAGMENTS "psram.lf" "registry.lf")
target_compile_definitions(${COMPONENT_LIB} PRIVATE NOASM DISABLE_SSE NO_FMOD NO_OPENAL NO_GTK
    __forceinline=inline)
target_compile_options(${COMPONENT_LIB} PRIVATE -include "${PROBE}/esp_compat.h"
    # Keep legacy diagnostics visible; exempt the two known warning classes.
    # The harness/backend files retain normal IDF warning policy.
    -Wno-error -Wno-error=class-memaccess -Wno-error=unused-but-set-parameter
    -Wno-register -fsigned-char -fstack-usage
    "$<$<COMPILE_LANGUAGE:CXX>:-std=gnu++11>")
set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 11)
list(LENGTH CORE_SOURCES CORE_COUNT)
message(STATUS "Experimental ZDoom core: ${CORE_COUNT} source units (full-link candidate)")
