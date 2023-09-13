#include "../xenobox.h"
#include "unlzop.h"
#include <zlib.h>

typedef struct
{
    unsigned char magic[4];                  // +0x00 : 'J','I','S','O'
    unsigned short version;                  // +0x04 : 0001
    unsigned short compression_block_size;   // +0x06 : uncompressed block size in byte
    unsigned short compression_block_header; // +0x08 : block header size in byte (not used, keeped for legacy support)
    unsigned short compression_algorithm;    // +0x0A : compression algorithm (lzo / zlib)
    unsigned int uncompressed_size;          // +0x0C : original iso file length in byte
    unsigned char md5_digest[16];            // +0x10 : md5 16-byte long digest
    unsigned int index_offset;               // +0x20 : file offset to index block
    unsigned short nNCareas;                 // +0x24 : number of non-compressed areas
    unsigned short reserved_short;           // +0x26 : reserved for futur use
    unsigned int NCareas_offset;             // +0x28 : file offset to NCArea block
    unsigned int reserved_long;              // +0x2C : reserved for futur use
} JISO_FH;                                   // +0x30 : end of header
#define JISO_HEADER_SIZE (0x30)

static int _compress(FILE* in, FILE* out, int level)
{
    fprintf(stderr, "compression won't be implemented\n");
    return -1;
}

static int _decompress(FILE* in, FILE* out)
{
    JISO_FH header;
    fread(&header, 1, sizeof(header), in);
    if (memcmp(header.magic, "JISO", 4))
    {
        fprintf(stderr, "not JISO\n");
        return 1;
    }
    if (header.nNCareas)
    {
        fprintf(stderr, "NC not supported\n");
        return 1;
    }
    if (header.compression_block_header)
    {
        fprintf(stderr, "compression block is legacy and unsupported\n");
        return 1;
    }
    int total_block = align2p(header.compression_block_size, header.uncompressed_size) / header.compression_block_size;
    fread(buf, 4, total_block + 1, in);
    int i = 0;
    int counter = sizeof(header) + 4 * (total_block + 1);
    for (; i < total_block; i++)
    {
        u32 index = read32(buf + 4 * i);
        u32 size = read32(buf + 4 * (i + 1)) - index;
        fread(__compbuf, 1, size, in);
        counter += size;

        if (header.compression_algorithm)
        {
            z_stream z;
            int status;

            z.zalloc = Z_NULL;
            z.zfree = Z_NULL;
            z.opaque = Z_NULL;

            if (inflateInit2(&z, -MAX_WBITS) != Z_OK)
            {
                fprintf(stderr, "inflateInit: %s\n", (z.msg) ? z.msg : "???");
                return 1;
            }

            z.next_in = __compbuf;
            z.avail_in = size;
            z.next_out = __decompbuf;
            z.avail_out = header.compression_block_size;

            status = inflate(&z, Z_FINISH);
            if (status != Z_STREAM_END && status != Z_OK)
            {
                fprintf(stderr, "inflate: %s\n", (z.msg) ? z.msg : "???");
                return 10;
            }

            if (inflateEnd(&z) != Z_OK)
            {
                fprintf(stderr, "inflateEnd: %s\n", (z.msg) ? z.msg : "???");
                return 2;
            }
            fwrite(__decompbuf, 1, header.compression_block_size - z.avail_out, out);
        }
        else
        {
            if (lzo1x_decompress_oneshot((char*)__decompbuf, header.compression_block_size, (char*)__compbuf, size))
                fprintf(stderr, "lzo decode error\n");
            fwrite(__decompbuf, 1, header.compression_block_size /*-z.avail_out*/, out);
        }
        if ((i + 1) % 256 == 0)
            fprintf(stderr, "%d / %d\r", i + 1, total_block);
    }
    fprintf(stderr, "%d / %d done.\n", i, total_block);
    return 0;
}

int xenojiso(const int argc, const char** argv)
{
    char mode, level;
    if (argc < 2)
        goto argerror;
    mode = argv[1][0], level = argv[1][1];
    if (!mode)
        goto argerror;
    if (mode == '-')
        mode = argv[1][1], level = argv[1][2];
    if (mode != 'e' && mode != 'c' && mode != 'd')
        goto argerror;
    if (isatty(fileno(stdin)) || isatty(fileno(stdout)))
        goto argerror;

    return mode == 'd' ? _decompress(stdin, stdout) : _compress(stdin, stdout, level ? level - '0' : 9);

argerror:
    fprintf(stderr, "xenojiso e/d < in > out\n"
                    "You can also use -e,-c,-d.\n"
                    "compression won't be available...\n");
    return -1;
}
