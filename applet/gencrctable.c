#include "../xenobox.h"

int gencrctable(const int argc, const char** argv)
{
    int bits;
    if (argc < 3)
    {
        fprintf(stderr, "gencrc32 8/16/32 poly (L)\n");
        return 1;
    }
    bits = strtoul(argv[1], NULL, 0);
    if (bits != 8 && bits != 16 && bits != 32)
    {
        fprintf(stderr, "gencrc32 8/16/32 poly (L)\n");
        return 1;
    }
    u32 poly = strtoul(argv[2], NULL, 0);
    u32 i = 0, j;

    printf("const unsigned ");
    if (bits == 8)
        puts("char crc8table[256]={");
    if (bits == 16)
        puts("short crc16table[256]={");
    if (bits == 32)
        puts("int crc32table[256]={");
    if (argc > 3)
    { // L
        for (; i < 256; i++)
        {
            unsigned int crc = i << (bits - 8);
            for (j = 0; j < 8; j++)
            {
                if (crc & (1 << (bits - 1)))
                    crc = (crc << 1) ^ poly;
                else
                    crc <<= 1;
            }
            if (bits == 8)
                printf("0x%02x,", crc & 0xff);
            if (bits == 16)
                printf("0x%04x,", crc & 0xffff);
            if (bits == 32)
                printf("0x%08x,", crc & 0xffffffff);
            if (i % 8 == 7)
                putchar('\n');
        }
    }
    else
    { // R
        for (; i < 256; i++)
        {
            unsigned int crc = i;
            for (j = 0; j < 8; j++)
            {
                if (crc & 1)
                    crc = (crc >> 1) ^ poly;
                else
                    crc >>= 1;
            }
            if (bits == 8)
                printf("0x%02x,", crc & 0xff);
            if (bits == 16)
                printf("0x%04x,", crc & 0xffff);
            if (bits == 32)
                printf("0x%08x,", crc & 0xffffffff);
            if (i % 8 == 7)
                putchar('\n');
        }
    }
    puts("};");
    return 0;
}
