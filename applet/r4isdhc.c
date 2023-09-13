#include "../xenobox.h"
#define S 433264

int r4isdhc(const int argc, const char** argv)
{
    FILE *in, *out;
    int size = S - 0x450, u;
    if (argc < 3)
    {
        fprintf(stderr, "r4isdhc in out\n");
        return 1;
    }
    in = fopen(argv[1], "rb");
    if (!in)
    {
        fprintf(stderr, "cannot open\n");
        return 2;
    }
    out = fopen(argv[2], "wb");
    if (!out)
    {
        fclose(in);
        fprintf(stderr, "cannot open\n");
        return 3;
    }
    memset(buf, 0, 0x450);
    buf[0] = 0x12, buf[1] = 0x01, buf[2] = 0x00, buf[3] = 0xea;
    buf[0xec] = 0x49, buf[0xed] = 0x00, buf[0xee] = 0x00, buf[0xef] = 0xeb;
    fwrite(buf, 1, 0x450, out);
    while (u = fread(buf, 1, BUFLEN, in))
        fwrite(buf, 1, u, out), size -= u;
    fclose(in);
    memset(buf, 0, BUFLEN);
    while (size > 0)
        fwrite(buf, 1, size < BUFLEN ? size : BUFLEN, out), size -= BUFLEN;
    fclose(out);
    return 0;
}
