#include "../../xenobox.h"

int compute_cksum(FILE* f, const char* name, const char* desired)
{
    u8 tmp[4];
    u32 crc = 0, totallen = 0;
    int readlen;
    if (f)
    {
        for (; (readlen = fread(buf, 1, BUFLEN, f)) > 0;)
        {
            crc = crc32_left(crc, buf, readlen);
            totallen += readlen;
        }
        write32(tmp, totallen);
        for (readlen = 0; totallen; totallen >>= 8)
            readlen++;
        crc = crc32_left(crc, tmp, readlen);
    }
    else
    {
        crc = crc32_left(crc, (unsigned char*)name, strlen(name));
        totallen = strlen(name);
        write32(tmp, totallen);
        for (readlen = 0; totallen; totallen >>= 8)
            readlen++;
        crc = crc32_left(crc, tmp, readlen);
    }
    if (desired)
        printf("%s: %s\n", name, strtoul(desired, NULL, 16) == crc ^ 0xffffffff ? "OK" : "NG");
    else
        printf("%08x  %s\n", crc ^ 0xffffffff, name);
    return 0;
}
