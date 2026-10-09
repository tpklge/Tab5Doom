#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "zlib.h"
#include "bzlib.h"
#include "LzmaDec.h"
#include "7zCrc.h"
#include "compression_fixtures.h"

/* The full engine supplies its own fatal-error handler in files.cpp.
 * The standalone probe must also fail hard on an internal library invariant. */
__attribute__((weak)) void bz_internal_error(int code)
{
    fprintf(stderr, "bzip2 internal invariant failed: %d\n", code);
    abort();
}

static void *alloc_bytes(void *context, size_t size)
{
    (void)context;
    return malloc(size);
}
static void free_bytes(void *context, void *address)
{
    (void)context;
    free(address);
}

/* Same probe runs on the host and in app_main. Nonzero identifies failure. */
int chex_compression_probe(void)
{
    unsigned char output[sizeof(fixture_plain)];
    uLongf zsize = sizeof(output);
    if (uncompress(output, &zsize, fixture_zlib, sizeof(fixture_zlib)) != Z_OK ||
        zsize != sizeof(output) || memcmp(output, fixture_plain, sizeof(output)))
        return 1;
    unsigned int bsize = sizeof(output);
    if (BZ2_bzBuffToBuffDecompress((char *)output, &bsize,
            (char *)fixture_bzip2, sizeof(fixture_bzip2), 0, 0) != BZ_OK ||
        bsize != sizeof(output) || memcmp(output, fixture_plain, sizeof(output)))
        return 2;
    ISzAlloc allocator = {alloc_bytes, free_bytes};
    SizeT outsize = sizeof(output), insize = sizeof(fixture_lzma);
    ELzmaStatus status;
    if (LzmaDecode(output, &outsize, fixture_lzma, &insize,
            fixture_lzma_props, sizeof(fixture_lzma_props), LZMA_FINISH_END,
            &status, &allocator) != SZ_OK || outsize != sizeof(output) ||
        status != LZMA_STATUS_FINISHED_WITH_MARK ||
        memcmp(output, fixture_plain, sizeof(output)))
        return 3;
    CrcGenerateTable();
    if (CrcCalc(fixture_plain, sizeof(fixture_plain)) !=
        crc32(0, fixture_plain, sizeof(fixture_plain)))
        return 4;
    /* A damaged zlib header must fail rather than returning plausible data. */
    unsigned char damaged[sizeof(fixture_zlib)];
    memcpy(damaged, fixture_zlib, sizeof(damaged));
    damaged[0] = 0;
    zsize = sizeof(output);
    if (uncompress(output, &zsize, damaged, sizeof(damaged)) == Z_OK)
        return 5;
    return 0;
}

#ifdef CHEX_COMPRESSION_HOST_TEST
int main(void)
{
    int result = chex_compression_probe();
    printf("Compression regression: %s (%d)\n", result ? "FAIL" : "PASS", result);
    return result;
}
#endif
