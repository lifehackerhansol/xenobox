#ifndef LZOP_H
#define LZOP_H

#define F_ADLER32_D     0x00000001L
#define F_ADLER32_C     0x00000002L
#define F_STDIN         0x00000004L
#define F_H_EXTRA_FIELD 0x00000040L
#define F_CRC32_D       0x00000100L
#define F_CRC32_C       0x00000200L
#define F_H_FILTER      0x00000800L
#define F_H_CRC32       0x00001000L
#define F_H_PATH        0x00002000L

#define V1 0x0900
#define V2 0x0940

typedef struct
{
    char name[256];
    unsigned short ver;
    unsigned short libver;
    unsigned short exver;
    unsigned char method;
    unsigned char level;
    unsigned int flags;
    unsigned char os;
    unsigned int filter;
    unsigned int mode;
    unsigned int mthigh;
    unsigned int mtlow;
} header;

typedef struct
{
    FILE* f;
    unsigned int adler32;
    unsigned int crc32;
} reader;

// int lzo1xy_decompress(memstream *mdec, memstream *menc, int lzo1y);
int lzo1x_decompress_oneshot(char* o_buff, int o_size, const char* i_buff, int i_size);

#endif
