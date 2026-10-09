#!/bin/sh
# Run after sourcing ESP-IDF export.sh. Does not link or flash firmware.
set -eu
cd "$(dirname "$0")/../.."
probe_compile() {
    riscv32-esp-elf-g++ -march=rv32imafc -mabi=ilp32f -std=gnu++11 \
        -DESP_PLATFORM -DNOASM -DDISABLE_SSE -DNDEBUG -fsigned-char -Os -ffunction-sections -fdata-sections \
        -include research/probe/esp_compat.h \
        -I research/zdoom-2.8.1/src -I research/zdoom-2.8.1/src/posix \
        -I research/zdoom-2.8.1/zlib -I research/zdoom-2.8.1/bzip2 \
        -I research/zdoom-2.8.1/src/sound \
        -I research/zdoom-2.8.1/src/g_shared -I research/zdoom-2.8.1/src/g_doom \
        -I research/zdoom-2.8.1/src/g_hexen -I research/zdoom-2.8.1/src/g_heretic \
        -I research/zdoom-2.8.1/src/g_raven -I research/zdoom-2.8.1/src/g_strife \
        -I research/zdoom-2.8.1/src/textures -I research/zdoom-2.8.1/src/thingdef \
        -c "$1" -o "$2"
}
probe_compile research/probe/fixed_probe.cpp research/probe/fixed_probe.o
probe_compile research/zdoom-2.8.1/src/sfmt/SFMT.cpp research/probe/sfmt.o
probe_compile research/zdoom-2.8.1/src/resourcefiles/file_wad.cpp research/probe/file_wad.o
probe_compile research/zdoom-2.8.1/src/r_draw.cpp research/probe/r_draw.o
riscv32-esp-elf-size research/probe/fixed_probe.o research/probe/sfmt.o research/probe/file_wad.o research/probe/r_draw.o
