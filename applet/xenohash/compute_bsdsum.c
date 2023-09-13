#include "../../xenobox.h"

int compute_bsdsum(FILE* f, const char* name, const char* desired)
{
    u16 crc = 0;
    int i;
    if (f)
    {
        int readlen;
        for (; (readlen = fread(buf, 1, BUFLEN, f)) > 0;)
        {
            for (i = 0; i < readlen; i++)
            {
                crc = lrotr(crc, 1) + buf[i];
            }
        }
    }
    else
    {
        for (i = 0; i < strlen(name); i++)
        {
            crc = lrotr(crc, 1) + ((unsigned char*)name)[i];
        }
    }

    if (desired)
        printf("%s: %s\n", name, strtoul(desired, NULL, 16) == crc ? "OK" : "NG");
    else
        printf("%04x  %s\n", crc, name);
    return 0;
}
