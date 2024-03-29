#ifdef STANDALONE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "zlib/zlib.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long int u64;
#define align2p(p, i) (((i) + ((p)-1)) & ~((p)-1))

#define BUFLEN (1 << 22)
unsigned char buf[BUFLEN];
#define cbuf ((char*)buf)

#define DECOMPBUFLEN (1 << 16)
#define COMPBUFLEN   (DECOMPBUFLEN | (DECOMPBUFLEN >> 1))
unsigned char __compbuf[COMPBUFLEN], __decompbuf[DECOMPBUFLEN];

unsigned int read32(const void* p)
{
    const unsigned char* x = (const unsigned char*)p;
    return x[0] | (x[1] << 8) | (x[2] << 16) | (x[3] << 24);
}
void write32(void* p, const unsigned int n)
{
    unsigned char* x = (unsigned char*)p;
    x[0] = n & 0xff, x[1] = (n >> 8) & 0xff, x[2] = (n >> 16) & 0xff, x[3] = (n >> 24) & 0xff;
}

#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
#else
int filelength(int fd)
{ // constant phrase
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}
#endif
#endif

#include "lzma.h"

typedef struct ciso_header
{
    unsigned char magic[4];             /* +00 : 'C','I','S','O'                 */
    unsigned int header_size;           /* +04 : header size (==0x18)            */
    unsigned long long int total_bytes; /* +08 : number of original data size    */
    unsigned int block_size;            /* +10 : number of compressed block size */
    unsigned char ver;                  /* +14 : version 01                      */
    unsigned char align;                /* +15 : align of index value            */
    unsigned char rsv_06[2];            /* +16 : reserved                        */
} CISO_H;

static int _compress(FILE* in, FILE* out, int level, int threshold)
{ // align not supported yet...
    CISO_H header;
    memset(&header, 0, sizeof(header));
    memcpy(header.magic, "CISO", 4);
    header.header_size = sizeof(header);
    header.total_bytes = filelength(fileno(in));
    header.block_size = 2048;
    header.ver = 1;
    header.align = 0;

    int total_block = align2p(header.block_size, header.total_bytes) / header.block_size;
    memset(buf, 0, 4 * (total_block + 1));
    fwrite(&header, 1, sizeof(header), out);
    fwrite(buf, 1, 4 * (total_block + 1), out);

#ifndef STANDALONE
    Options options;
    if (level > 9)
    {
        InitOptions(&options);
        options.numiterations = level / 10;
    }
    else
#endif

        void* coder = NULL;
    lzmaCreateCoder(&coder, 0x040108, 1, level);
    int i = 0;
    for (; i < total_block; i++)
    {
        write32(buf + 4 * i, ftell(out));
        size_t readlen = fread(__decompbuf, 1, header.block_size, in);
        size_t compsize = 0;
        unsigned char* COMPBUF = __compbuf;
#ifndef STANDALONE
        if (level > 9)
        {
            size_t __compsize = 0;
            unsigned char bp = 0;
            COMPBUF = NULL;
            Deflate(&options, 2 /* Dynamic block */, 1, __decompbuf, (size_t)readlen, &bp, &COMPBUF, &__compsize);
            compsize = __compsize;
        }
        else

#endif
            if (coder)
        {
            compsize = COMPBUFLEN;
            int r = lzmaCodeOneshot(coder, __decompbuf, readlen, COMPBUF, &compsize);
            if (r)
            {
                fprintf(stderr, "NDeflate::CCoder::Code %d\n", r);
                return 1;
            }
        }
        else
        {
            z_stream z;
            int status;
            // int flush=Z_NO_FLUSH;

            z.zalloc = Z_NULL;
            z.zfree = Z_NULL;
            z.opaque = Z_NULL;

            if (deflateInit2(&z, level, Z_DEFLATED, -MAX_WBITS, level, Z_DEFAULT_STRATEGY) != Z_OK)
            {
                fprintf(stderr, "deflateInit: %s\n", (z.msg) ? z.msg : "???");
                return 1;
            }

            z.next_in = __decompbuf;
            z.avail_in = readlen;
            z.next_out = __compbuf;
            z.avail_out = 65536 * 2;

            status = deflate(&z, Z_FINISH);
            if (status != Z_STREAM_END && status != Z_OK)
            {
                fprintf(stderr, "inflate: %s\n", (z.msg) ? z.msg : "???");
                return 10;
            }
            compsize = 65536 * 2 - z.avail_out;

            if (deflateEnd(&z) != Z_OK)
            {
                fprintf(stderr, "inflateEnd: %s\n", (z.msg) ? z.msg : "???");
                return 2;
            }
        }
        if (compsize > header.block_size * threshold / 100)
        { // store
            fwrite(__decompbuf, 1, readlen, out);
            write32(buf + 4 * i, 0x80000000 | read32(buf + 4 * i));
        }
        else
        {
            fwrite(COMPBUF, 1, compsize, out);
        }
#ifndef STANDALONE
        if (level > 9)
            free(COMPBUF);
#endif
        if ((i + 1) % 256 == 0)
            fprintf(stderr, "%d / %d\r", i + 1, total_block);
    }
    write32(buf + 4 * i, ftell(out));
    fseek(out, sizeof(header), SEEK_SET);
    fwrite(buf, 1, 4 * (total_block + 1), out);
    fprintf(stderr, "%d / %d done.\n", i, total_block);
    lzmaDestroyCoder(&coder);
    return 0;
}

static int _decompress(FILE* in, FILE* out)
{
    CISO_H header;
    fread(&header, 1, sizeof(header), in);
    if (memcmp(header.magic, "CISO", 4) || (header.header_size && header.header_size != sizeof(header)))
    {
        fprintf(stderr, "not CISO\n");
        return 1;
    }
    int total_block = align2p(header.block_size, header.total_bytes) / header.block_size;
    fread(buf, 4, total_block + 1, in);
    int i = 0;
    int counter = sizeof(header) + 4 * (total_block + 1);

    // void* coder=NULL;
    // lzmaCreateCoder(&coder,0x040108,0,0);
    for (; i < total_block; i++)
    {
        u32 index = read32(buf + 4 * i);
        u32 plain = index & 0x80000000;
        index &= 0x7fffffff;
        u32 pos = index << header.align;
        if (pos > counter)
            fread(buf + BUFLEN - (pos - counter), 1, pos - counter, in);

        u32 size = ((read32(buf + 4 * (i + 1)) & 0x7fffffff) << header.align) - index;
        if (plain)
        {
            fread(__decompbuf, 1, size, in);
            counter += size;
            fwrite(__decompbuf, 1, size, out);
        }
        else
        {
            fread(__compbuf, 1, size, in);
            counter += size;
            size_t decompsize = header.block_size;
            // if(coder){
            //		int r=lzmaCodeOneshot(coder,__compbuf,size,__decompbuf,&decompsize);
            //		if(r){
            //			fprintf(stderr,"NDeflate::CCoder::Code %d\n",r);
            //			return 1;
            //		}
            // }else
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
                z.avail_out = header.block_size;

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
                decompsize = header.block_size - z.avail_out;
            }
            fwrite(__decompbuf, 1, decompsize, out);
        }
        if ((i + 1) % 256 == 0)
            fprintf(stderr, "%d / %d\r", i + 1, total_block);
    }
    fprintf(stderr, "%d / %d done.\n", i, total_block);
    // lzmaDestroyCoder(&coder);
    return 0;
}

int main(const int argc, const char** argv)
{
    int mode;
    if (argc < 2)
        goto argerror;
    mode = argv[1][0];
    if (!mode)
        goto argerror;
    if (mode == 'd')
    {
        if (isatty(fileno(stdin)) || isatty(fileno(stdout)))
            goto argerror;
        // lzmaOpen7z();
        int ret = _decompress(stdin, stdout);
        // lzmaClose7z();
        return ret;
    }
    else if ((mode = strtol(argv[1], NULL, 0)) > 0)
    {
        fprintf(stderr, "compression level = %d\n", mode);
        // if(mode>9)fprintf(stderr,"(zopfli numiterations %d)\n",mode/10);
        // else fprintf(stderr,"(zlib)\n");
        int threshold = 100;
        if (argc > 4)
            threshold = strtol(argv[4], NULL, 0);
        if (threshold < 10)
            threshold = 10;
        if (threshold > 100)
            threshold = 100;
        FILE* in = fopen(argv[2], "rb");
        if (!in)
        {
            fprintf(stderr, "failed to open %s\n", argv[2]);
            return 2;
        }
        FILE* out = fopen(argv[3], "wb");
        if (!out)
        {
            fclose(in);
            fprintf(stderr, "failed to open %s\n", argv[3]);
            return 2;
        }
        lzmaOpen7z();
        int ret = _compress(in, out, mode, threshold);
        lzmaClose7z();
        fclose(in), fclose(out);
        return ret;
    }

argerror:
    fprintf(stderr, "7ciso level enc dec [threshold(%%,10-100)]\n"
                    "7ciso d < enc > dec\n");
    return -1;
}
