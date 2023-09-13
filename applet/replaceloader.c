#include "../xenobox.h"

#define M 4
static const byte loadermagic[M][17] = {
    "\x02\0\0\xea\0\0\0\0\x01\0\0\0\0\0\0\0",           // v1
    "\x05\0\0\xea\xff\xff\xff\x0f\x01\0\0\0\x01\0\0\0", // v2
    "\x05\0\0\xea\0\0\0\0\x01\0\0\0\0\0\0\0",
    "\x06\0\0\xea\xff\xff\xff\x0f\x01\0\0\0\x01\0\0\0", // v3
};

static int patch(byte* nds, const int ndslen, const byte* pD, const int loaderlen)
{
    byte* pA = NULL;
    int i, j;

    for (i = 0; i < ndslen - loaderlen; i += 4)
    {
        for (j = 0; j < M; j++)
            if (!memcmp(nds + i, loadermagic[j], 16) && pD[0] >= nds[i])
            {
                pA = nds + i;
                goto patch;
            }
    }
    /*if(!pA)*/ {
        printf("not found loader section or loader revision is older\n");
        return 1;
    }

patch:
    memcpy(pA, pD, loaderlen);
    printf("Patched successfully\n");
    return 0;
}

int replaceloader(const int argc, const char** argv)
{
    int i;
    FILE *f, *fl;
    struct stat st, stl;
    byte *p, *pl;

    if (argc < 3)
    {
        printf("replaceloader\n");
        printf("replaceloader load.bin homebrew...\n");
        return 1;
    }
    if (!(fl = fopen(argv[1], "rb")))
    {
        printf("cannot open %s\n", argv[1]);
        return 2;
    }
    fstat(fileno(fl), &stl);
    if (!(pl = malloc(stl.st_size)))
    {
        fclose(fl);
        printf("cannot allocate %d bytes for load.bin\n", (int)stl.st_size);
        return 3;
    }
    fread(pl, 1, stl.st_size, fl);
    fclose(fl);
    for (i = 0; i < M; i++)
        if (!memcmp(pl, loadermagic[i], 16))
            break;
    if (i == M)
    {
        printf("the loader is invalid\n");
        return 4;
    }

    for (i = 2; i < argc; i++)
    {
        printf("Patching %s...\n", argv[i]);
        if (!(f = fopen(argv[i], "rb+")))
        {
            printf("cannot open %s\n", argv[i]);
            continue;
        }
        fstat(fileno(f), &st);
        if (!(p = malloc(st.st_size)))
        {
            printf("cannot allocate %d bytes for %s\n", (int)st.st_size, argv[i]);
            continue;
        }
        fread(p, 1, st.st_size, f);
        rewind(f);
        if (!patch(p, st.st_size, pl, stl.st_size))
            fwrite(p, 1, st.st_size, f);
        fclose(f);
    }
    free(pl);
    return 0;
}
