# Explicit upstream library units, shared by IDF and native regression builds.
# No desktop CMake configure checks or x86 assembly are used.
set(ZD_COMPRESSION_SOURCES)
foreach(FILE adler32 compress crc32 deflate inflate infback inftrees inffast trees uncompr zutil)
    list(APPEND ZD_COMPRESSION_SOURCES "${ZD_ROOT}/zlib/${FILE}.c")
endforeach()
if(ZD_EXTRA_DECODERS)
foreach(FILE jcomapi jdapimin jdapistd jdatasrc jdcoefct jdcolor jddctmgr jdhuff
             jdinput jdmainct jdmarker jdmaster jdmerge jdphuff jdpostct jdsample
             jerror jidctint jmemmgr jutils)
    list(APPEND ZD_COMPRESSION_SOURCES "${ZD_ROOT}/jpeg-6b/${FILE}.c")
endforeach()
foreach(FILE dmisc dtoa misc)
    list(APPEND ZD_COMPRESSION_SOURCES "${ZD_ROOT}/gdtoa/${FILE}.c")
endforeach()
endif()
foreach(FILE blocksort bzlib compress crctable decompress huffman randtable)
    list(APPEND ZD_COMPRESSION_SOURCES "${ZD_ROOT}/bzip2/${FILE}.c")
endforeach()
foreach(FILE 7zArcIn 7zBuf 7zCrc 7zCrcOpt 7zDec 7zStream Bcj2 Bra Bra86 BraIA64
             CpuArch Delta LzFind Lzma2Dec LzmaDec LzmaEnc Ppmd7 Ppmd7Dec)
    list(APPEND ZD_COMPRESSION_SOURCES "${ZD_ROOT}/lzma/C/${FILE}.c")
endforeach()

