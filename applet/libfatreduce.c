#include "../xenobox.h"

int libfatreduce(const int argc, const char** argv)
{
    unsigned char* ys;
    unsigned size, ext;
    FILE* f;

    // startup
    fprintf(stderr, "libfatreduce\n");
    if (argc < 2)
    {
        fprintf(stderr, "specify path for nds\n");
        return 1;
    }
    ext = strlen(argv[1]);
    if (ext < 4 || argv[1][ext - 4] != '.')
    {
        fprintf(stderr, "file has to have extention such as .nds or .dat\n");
        return 1;
    }
    ext -= 3;

    // read
    f = fopen(argv[1], "rb");
    if (!f)
    {
        fprintf(stderr, "cannot open ysmenu\n");
        return 2;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size > 0x2000000)
    {
        fprintf(stderr, "too big\n");
        fclose(f);
        return 3;
    }
    ys = (unsigned char*)malloc(size);
    if (!ys)
    {
        fprintf(stderr, "cannot alloc memory\n");
        fclose(f);
        return 4;
    }
    fread(ys, 1, size, f);
    fclose(f);

    // patch
    {
        int i = 0x200, p = 0;
        for (; i < size - 100; i += 2)
        {
            if (ys[i + 1] == 0x28 && ys[i] == 0x00 &&     // cmp r0,#0x0
                ys[i + 3] == 0xd1 && ys[i + 2] == 0x06 && // bne +6
                ys[i + 5] == 0x35 && ys[i + 4] == 0x01 && // add r5,#0x1
                ys[i + 7] == 0x36 && ys[i + 6] == 0x04 && // add r6,#0x4
                ys[i + 9] == 0x34 && ys[i + 8] == 0x04 && // add r6,#0x4
                ys[i + 11] == 0x2d &&                     // cmp r5,#0x8
                ys[i + 13] == 0xd1                        // bne ...
            )
            {
                // if(p){fprintf(stderr,"multiple hits (patch error)\n");free(ys);return -1;}
                fprintf(stderr, "Patched 0x%08x (%d)\n", i + 0x10, ys[i + 10]);
                ys[p = i + 10] = 0x01; // cmp r5,#0x1
            }
        }
        if (!p)
        {
            fprintf(stderr, "no hits (patch error)\n");
            free(ys);
            return -1;
        }
    }

    // write
    {
        char* n = (char*)malloc(strlen(argv[1]) + 1);
        strcpy(n, argv[1]);
        n[ext] = 'b';
        n[ext + 1] = 'a';
        n[ext + 2] = 'k';
        rename(argv[1], n);
        free(n);
    }
    f = fopen(argv[1], "wb");
    fwrite(ys, 1, size, f);
    fclose(f);
    free(ys);
    fprintf(stderr, "patch OK\n");
    return 0;
}
