#include "../../xenobox.h"

int compute_elf32(FILE* f, const char* name, const char* desired)
{
    u32 crc = 0;
    int i;
    if (f)
    {
        int readlen;
        for (; (readlen = fread(buf, 1, BUFLEN, f)) > 0;)
        {
            for (i = 0; i < readlen; i++)
            {
                crc = (crc << 4) + buf[i];
                u32 x = crc & 0xf0000000;
                if (x)
                    crc ^= x >> 24;
                crc &= ~x;
            }
        }
    }
    else
    {
        for (i = 0; i < strlen(name); i++)
        {
            crc = (crc << 4) + ((unsigned char*)name)[i];
            u32 x = crc & 0xf0000000;
            if (x)
                crc ^= x >> 24;
            crc &= ~x;
        }
    }

    if (desired)
        printf("%s: %s\n", name, strtoul(desired, NULL, 16) == crc ? "OK" : "NG");
    else
        printf("%08x  %s\n", crc, name);
    return 0;
}
