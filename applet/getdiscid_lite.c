#include "../xenobox.h"

int getdiscid_lite(const int argc, const char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "getdiscid_lite DISC.iso\n");
        return -1;
    }
    FILE* f = fopen(argv[1], "rb");
    if (!f)
    {
        fprintf(stderr, "fopen failed\n");
        return 2;
    }
    fseek(f, 0x8050, SEEK_SET);
    fread(buf, 1, 16, f);
    if (buf[0] != buf[7] || buf[1] != buf[6] || buf[2] != buf[5] || buf[3] != buf[4])
    {
        fclose(f);
        fprintf(stderr, "not valid ISO\n");
        return 3;
    }
    fseek(f, 0x8028, SEEK_SET);
    fread(buf, 1, 80, f);
    if (*buf && *buf != 0x20)
    {
        int i = 0;
        fprintf(stderr, "ISO Label=");
        for (; buf[i] && buf[i] != 0x20; i++)
            fputc(buf[i], stderr);
        fputc('\n', stderr);
    }
    fseek(f, 0x8373, SEEK_SET);
    fread(buf, 1, 10, f);
    buf[10] = 0;
    fclose(f);
    puts(cbuf);
    return 0;
}
