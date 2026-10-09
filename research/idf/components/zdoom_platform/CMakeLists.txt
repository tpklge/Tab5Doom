get_filename_component(PROBE "${CMAKE_CURRENT_LIST_DIR}/../../../probe" ABSOLUTE)
idf_component_register(WHOLE_ARCHIVE SRCS "i_timer.cpp" "i_system.cpp" "i_files.cpp" "i_music.cpp" "i_video.cpp" "i_input.cpp" "i_audio.cpp" INCLUDE_DIRS "."
    REQUIRES zdoom_core esp_timer freertos tab5_hardware)
target_compile_options(${COMPONENT_LIB} PRIVATE -include "${PROBE}/esp_compat.h"
    -Wno-error=class-memaccess -fsigned-char "$<$<COMPILE_LANGUAGE:CXX>:-std=gnu++11>")
