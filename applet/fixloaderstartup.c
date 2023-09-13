#include "../xenobox.h"

int fixloaderstartup(const int argc, const char** argv)
{
    unsigned char* ys;
    unsigned size, ext;
    FILE* f;

    // startup
    fprintf(stderr, "fixloaderstartup\n");
    if (argc < 2)
    {
        fprintf(stderr, "specify path for homebrew\n");
        return 1;
    }
    ext = strlen(argv[1]);
    if (ext < 4 || argv[1][ext - 4] != '.')
    {
        fprintf(stderr, "homebrew has to have extention such as .nds or .dat\n");
        return 1;
    }
    ext -= 3;

    // read
    f = fopen(argv[1], "rb");
    if (!f)
    {
        fprintf(stderr, "cannot open homebrew\n");
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
            if (ys[i + 3] == 0xe5 && ys[i + 2] == 0x9f &&                                               //
                ys[i + 7] == 0xe3 && ys[i + 6] == 0xa0 && ys[i + 5] == 0x20 /*&&ys[i+ 4]==0x08*/ &&     //
                ys[i + 11] == 0xeb && ys[i + 10] == 0x00 &&                                             //
                ys[i + 15] == 0xe3 && ys[i + 14] == 0xa0 && ys[i + 13] == 0x00 && ys[i + 12] == 0x01 && //
                ys[i + 19] == 0xea && ys[i + 18] == 0x00 && ys[i + 17] == 0x00 && ys[i + 16] == 0x00 && //
                ys[i + 23] == 0xe3 && ys[i + 22] == 0xa0 && ys[i + 21] == 0x00 && ys[i + 20] == 0x00    //
            )
            {
                if (p)
                {
                    fprintf(stderr, "multiple hits (patch error)\n");
                    free(ys);
                    return -1;
                }
                p = i + 4;
            }
        }
        if (!p)
        {
            fprintf(stderr, "no hits (patch error)\n");
            free(ys);
            return -1;
        }
        if (ys[p] != 0x08)
        {
            if (ys[p] == 0x00)
            {
                fprintf(stderr, "already patched\n");
                free(ys);
                return -1;
            }
            fprintf(stderr, "incorrect detection (patch error)\n");
            free(ys);
            return -1;
        }
        ys[p] = 0x00;
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
