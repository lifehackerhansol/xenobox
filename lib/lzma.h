#ifndef _LZMA_H_
#define _LZMA_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#ifdef FEOS
    typedef long fpos_t;
#endif
    // from BSD libc
    typedef int (*tRead)(void*, char*, int);
    typedef int (*tWrite)(void*, const char*, int);
    typedef fpos_t (*tSeek)(void*, fpos_t, int);
    typedef int (*tClose)(void*);

#include <stdbool.h>
    int lzmaOpen7z();
    bool lzma7zAlive();
    int lzmaClose7z();
    // Please see guid.txt
    int lzmaGUIDSetArchiver(void* g, unsigned char arctype);
    /*
    0x040108 Deflate
    0x040109 Deflate64
    0x040202 BZip2
    0x030401 PPMD
    0x030101 LZMA
    0x21     LZMA2
    0x00     Copy
    */
    int lzmaGUIDSetCoder(void* g, unsigned long long int codecid, int encode);
    int lzmaCreateArchiver(void** archiver, unsigned char arctype, int encode, int level);
    int lzmaDestroyArchiver(void** archiver, int encode);
    int lzmaCreateCoder(void** coder, unsigned long long int id, int encode, int level);
    int lzmaDestroyCoder(void** coder);
    int lzmaCodeOneshot(void* coder, unsigned char* in, size_t isize, unsigned char* out, size_t* osize);
    int lzmaCodeCallback(void* coder, void* hin, tRead pRead_in, tClose pClose_in, void* hout, tWrite pWrite_out,
                         tClose pClose_out);

#ifdef __cplusplus
}
#endif

#endif
