#include "../../xenobox.h"

int compute_bz2crc32(FILE* f, const char* name, const char* desired)
{
    u32 crc = 0xffffffff;
    if (f)
    {
        int readlen;
        for (; (readlen = fread(buf, 1, BUFLEN, f)) > 0;)
        {
            crc = crc32_left(crc, buf, readlen);
        }
    }
    else
    {
        crc = crc32_left(crc, (unsigned char*)name, strlen(name));
    }
    if (desired)
        printf("%s: %s\n", name, strtoul(desired, NULL, 16) == crc ^ 0xffffffff ? "OK" : "NG");
    else
        printf("%08x  %s\n", crc ^ 0xffffffff, name);
    return 0;
}
