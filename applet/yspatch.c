#include "../xenobox.h"

int yspatch(const int argc, const char** argv)
{
    unsigned char* ys;
    unsigned size, ext;
    FILE* f;

    // startup
    fprintf(stderr, "yspatch rev. ALPHA\n");
    if (argc < 2)
    {
        fprintf(stderr, "specify path for ysmenu\n");
        return 1;
    }
    ext = strlen(argv[1]);
    if (ext < 4 || argv[1][ext - 4] != '.')
    {
        fprintf(stderr, "ysmenu has to have extention such as .nds or .dat\n");
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
        for (; i < size - 100; i += 4)
        {
            if (ys[i + 3] == 0xe1 && ys[i + 2] == 0xa0 && ys[i + 1] == 0x00 && ys[i] == 0x02 &&         // mov r0,r2
                ys[i + 7] == 0xe0 && ys[i + 6] == 0x62 && ys[i + 5] == 0x40 && ys[i + 4] == 0x03 &&     // rsb r4,r2,r3
                ys[i + 11] == 0xe1 && ys[i + 10] == 0xa0 && ys[i + 9] == 0x10 && ys[i + 8] == 0x04 &&   // mov r1,r4
                ys[i + 15] == 0xeb &&                                                                   // bl addr...
                ys[i + 19] == 0xe5 &&                                                                   // ldr ...
                ys[i + 23] == 0xe5 &&                                                                   // ldr ...
                ys[i + 27] == 0xe1 && ys[i + 26] == 0x50 && ys[i + 25] == 0x00 && ys[i + 24] == 0x02 && // cmp r0,r2
                /*ys[i+31]==0x0a&&*/ ys[i + 30] == 0x00 && ys[i + 29] == 0x00 &&
                ys[i + 28] == 0x14 // beq ... (if not infinite-loop)
            )
            {
                if (p)
                {
                    fprintf(stderr, "multiple hits (patch error)\n");
                    free(ys);
                    return -1;
                }
                p = i + 31;
            }
        }
        if (!p)
        {
            fprintf(stderr, "no hits (patch error)\n");
            free(ys);
            return -1;
        }
        if (ys[p] != 0x0a)
        {
            if (ys[p] == 0xea)
            {
                fprintf(stderr, "already patched\n");
                free(ys);
                return -1;
            }
            fprintf(stderr, "incorrect detection (patch error)\n");
            free(ys);
            return -1;
        }
        ys[p] = 0xea; // b (never infinite-loop)
    }

    // write
    /*
      {
        char *n=(char*)malloc(strlen(argv[1])+1);
        strcpy(n,argv[1]);
        n[ext]='b';n[ext+1]='a';n[ext+2]='k';
        rename(argv[1],n);
        free(n);
      }
    */
    f = fopen(argv[1], "wb");
    fwrite(ys, 1, size, f);
    fclose(f);
    free(ys);
    fprintf(stderr, "patch OK\n");
    return 0;
}
