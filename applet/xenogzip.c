#include "../xenobox.h"

int xenogzip(const int argc, const char** argv)
{
    int readlen;
    gzFile gz;

    if (isatty(fileno(stdin)))
    {
        fprintf(stderr, "xenogzip [d] < in > out\n");
        return -1;
    }

    if (argc > 1)
    {
        gz = gzdopen(fileno(stdin), "rb");
        for (; (readlen = gzread(gz, buf, BUFLEN)) > 0;)
        {
            fwrite(buf, 1, readlen, stdout);
        }
    }
    else
    {
        gz = gzdopen(fileno(stdout), "wb9");
        for (; (readlen = fread(buf, 1, BUFLEN, stdin)) > 0;)
        {
            gzwrite(gz, buf, readlen);
        }
        gzflush(gz, Z_FINISH); // lol
    }
    return 0;
}
