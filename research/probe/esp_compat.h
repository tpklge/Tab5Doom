#pragma once
// Initial compatibility shim for isolated compile probes, not an engine backend.
#include <strings.h>
#include <float.h>
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "ZDoom ESP-IDF probe expects little-endian RV32"
#endif
#ifdef __cplusplus
static_assert(sizeof(void *) == 4 && sizeof(double) == 8,
              "Expected RV32 pointers and binary64 double");
static_assert(FLT_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024,
              "Expected IEEE binary64 arithmetic");
#endif
#define stricmp strcasecmp
#define strnicmp strncasecmp
