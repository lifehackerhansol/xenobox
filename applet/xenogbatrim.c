#include "../xenobox.h"

int xenogbatrim(const int argc, const char** argv)
{
    int i = 1; // 2;
    int j;

    FILE* f;
    // struct stat st;

    fprintf(stderr, "xenogbatrim\n");
    if (argc < i + 1)
    {
        fprintf(stderr, "Usage: xenogbatrim rom...\n"
                        "It might be able to be used for any cartridge type image, but no guarantee.\n");
        return -1;
    }
    for (; i < argc; i++)
    {
        fprintf(stderr, "%s: ", argv[i]);
        f = fopen(argv[i], "rb+");
        if (!f)
        {
            fprintf(stderr, "Cannot open\n");
            continue;
        }
        u32 s = filelength(fileno(f));
        if (s & 0xffff)
        {
            fclose(f);
            fprintf(stderr, "Not aligned to 0x10000; it seems already trimmed\n");
            continue;
        }
        s -= 0x10000;
        for (; s; s -= 0x10000)
        {
            fseek(f, s, SEEK_SET);
            fread(buf, 1, 0x10000, f);
            for (j = 65535; j >= 0; j--)
                if (buf[s] != 0 && buf[s] != 0xff)
                {
                    s += j + 1;
                    break;
                }
        }
        if (s)
        {
            fprintf(stderr, "Trim %08x -> %08x\n", (u32)filelength(fileno(f)), s);
            ftruncate(fileno(f), s);
        }
        else
        {
            fprintf(stderr, "Failed to trim\n");
        }
        fclose(f);
    }
    return 0;
}
