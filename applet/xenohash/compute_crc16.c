#include "../../xenobox.h"

int compute_crc16(FILE* f, const char* name, const char* desired)
{
    u16 crc = 0;
    if (f)
    {
        int readlen;
        for (; (readlen = fread(buf, 1, BUFLEN, f)) > 0;)
        {
            crc = crc16(crc, buf, readlen);
        }
    }
    else
    {
        crc = crc16(crc, (unsigned char*)name, strlen(name));
    }

    if (desired)
        printf("%s: %s\n", name, strtoul(desired, NULL, 16) == crc ? "OK" : "NG");
    else
        printf("%04x  %s\n", crc, name);
    return 0;
}
