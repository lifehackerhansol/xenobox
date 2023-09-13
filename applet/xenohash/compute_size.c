#include "../../xenobox.h"

int compute_size(FILE* f, const char* name, const char* desired)
{
    u32 totallen = 0;
    if (f)
    {
        int readlen;
        for (; (readlen = fread(buf, 1, BUFLEN, f)) > 0;)
        {
            totallen += readlen;
        }
    }
    else
    {
        totallen = strlen(name);
    }
    if (desired)
        printf("%s: %s\n", name, strtoul(desired, NULL, 10) == totallen ? "OK" : "NG");
    else
        printf("%d  %s\n", totallen, name);
    return 0;
}
