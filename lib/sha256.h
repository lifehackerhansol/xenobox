#ifndef SHA256_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    /// Buffer to hold the final result and a temporary buffer for SHA256.
    union
    {
        uint8_t u8[64];
        uint32_t u32[16];
        uint64_t u64[8];
    } buffer;

    /// Check-specific data
    union
    {
        uint32_t crc32;
        uint64_t crc64;

        struct
        {
            /// Internal state
            uint32_t state[8];

            /// Size of the message excluding padding
            uint64_t size;
        } sha256;
    } state;

} lzma_check_state;

void lzma_sha256_init(lzma_check_state* check);
void lzma_sha256_update(const uint8_t* buf, size_t size, lzma_check_state* check);
void lzma_sha256_finish(lzma_check_state* check);

#ifdef FEOS
#define UINT32_C(n) (n##UL)
#define UINT64_C(n) (n##ULL)
#endif
#endif
