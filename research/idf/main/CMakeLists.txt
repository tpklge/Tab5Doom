idf_component_register(SRCS "main.cpp" "../../probe/compression_probe.c"
    INCLUDE_DIRS "." REQUIRES zdoom_core zdoom_compression zdoom_platform esp_psram tab5_hardware)

get_filename_component(PROBE "${CMAKE_CURRENT_LIST_DIR}/../../probe" ABSOLUTE)
target_compile_options(${COMPONENT_LIB} PRIVATE -include "${PROBE}/esp_compat.h" -fsigned-char "$<$<COMPILE_LANGUAGE:CXX>:-std=gnu++11>")
