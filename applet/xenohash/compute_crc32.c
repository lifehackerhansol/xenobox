#include "../../xenobox.h"
#include <zlib.h>

// clone of /usr/bin/crc32 from CPAN Archive::Zip
int compute_crc32(FILE* f, const char* name, const char* desired)
{
    u32 crc = 0;
    if (f)
    {
        int readlen;
        for (; (readlen = fread(buf, 1, BUFLEN, f)) > 0;)
        {
            crc = crc32(crc, buf, readlen);
        }
    }
    else
    {
        crc = crc32(crc, (unsigned char*)name, strlen(name));
    }

    if (desired)
        printf("%s: %s\n", name, strtoul(desired, NULL, 16) == crc ? "OK" : "NG");
    else
        printf("%08x  %s\n", crc, name);
    return 0;
}
