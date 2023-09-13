#include "../xenobox.h"

static void encnum(u8* p, u64 num, u8** new)
{
    int f = 0;
    for (;;)
    {
        num -= f;
        u8 x = num & 0x7f;
        num >>= 7;
        *p = x;
        if (!num)
        {
            *p |= 0x80;
            break;
        }
        p++;
        f = 1;
    }
    p++;
    *new = p;
}

static u64 decnum(const u8* p, const u8** new)
{
    u64 ret = 0;
    u32 shift = 0;
    int f = 0;
    for (;;)
    {
        u8 x = *p;
        p++;
        ret += ((x & 0x7f) + f) << shift;
        if ((x & 0x80) == 0x80)
            break;
        shift += 7;
        f = 1;
    }
    *new = p;
    return ret;
}

static int upsmake(const u8* pfile, const unsigned int sizfile, const u8* pnewfile, const unsigned int siznewfile,
                   const unsigned int readsize, FILE* ups)
{
    u8 u8zero = 0;
    unsigned int crc = 0, _crc;
    fwrite("UPS1", 1, 4, ups);
    crc = crc32(crc, (u8*)"UPS1", 4);
    u8* p;
    encnum(buf, sizfile, &p);
    fwrite(buf, 1, p - buf, ups);
    crc = crc32(crc, buf, p - buf);
    encnum(buf, siznewfile, &p);
    fwrite(buf, 1, p - buf, ups);
    crc = crc32(crc, buf, p - buf);

    unsigned int i = 0, address = 0, final;

    unsigned int oldoff = 0;
    for (; i < readsize; i++)
    {
        if (pfile[i] != pnewfile[i])
        {
            address = final = i;
            for (; memcmp_fast(pfile + final, pnewfile + final, min(1, readsize - final)); final++)
                i++; // using gap isn't allowed

            encnum(buf, address - oldoff, &p);
            fwrite(buf, 1, p - buf, ups);
            crc = crc32(crc, buf, p - buf);
            for (; address < final; address++)
            {
                u8 x = pfile[address] ^ pnewfile[address];
                fwrite(&x, 1, 1, ups);
                crc = crc32(crc, &x, 1);
            }
            fwrite(&u8zero, 1, 1, ups);
            crc = crc32(crc, &u8zero, 1);
            oldoff = final + 1;
        }
    }

    _crc = crc32(0, pfile, sizfile);
    write32(buf, _crc);
    fwrite(buf, 1, 4, ups);
    crc = crc32(crc, buf, 4);
    _crc = crc32(0, pnewfile, siznewfile);
    write32(buf, _crc);
    fwrite(buf, 1, 4, ups);
    crc = crc32(crc, buf, 4);

    write32(buf, crc);
    fwrite(buf, 1, 4, ups);
    fflush(ups);
    return 0;
}

static int upspatch(u8* pfile, unsigned int sizfile, const u8* pups, const unsigned int sizups)
{ // pass pfile=NULL to get sizfile.
    if (sizups < 16 || memcmp(pups, "UPS1", 4))
        return 2;
    unsigned int crc = 0;
    crc = crc32(crc, pups, sizups - 4);
    if (crc != read32(pups + (sizups - 4)))
        return 2;

    const u8* new = NULL;
    /*u64 sizex=*/decnum(pups + 4, &new);
    /*u64 sizey=*/decnum(new, &new);

    while (1)
    {
        if (new == pups + sizups - 12)
            return 0;
        u64 relative = decnum(new, &new);
        if (new + relative + 1 > pups + sizups - 12)
            return 1;
        if (pfile)
        {
            pfile += relative;
            for (; *new; new ++)
                *pfile++ ^= *new;
            pfile++;
        }
        else
        {
            for (; *new; new ++)
                ;
        }
        new ++;
    }
    return 0;
}

// bootstrap
static int _upsmake(FILE* file, FILE* newfile, FILE* ups)
{
    u8 *pfile = NULL, *pnewfile = NULL;
    unsigned int sizfile, siznewfile;
    // int ret;

    sizfile = filelength(fileno(file));
    siznewfile = filelength(fileno(newfile));

    u32 readsize = max(sizfile, siznewfile);
    pfile = (u8*)malloc(readsize);
    pnewfile = (u8*)malloc(readsize);
    if (!pfile || !pnewfile)
    {
        if (pfile)
            free(pfile);
        if (pnewfile)
            free(pnewfile);
        fprintf(stderr, "cannot allocate memory\n");
        return 2;
    }
    memset(pfile, 0, readsize);
    fread(pfile, 1, sizfile, file);
    memset(pnewfile, 0, readsize);
    fread(pnewfile, 1, siznewfile, newfile);

    fprintf(stderr, "(%u bytes / %u bytes)\n", sizfile, siznewfile);
    upsmake(pfile, sizfile, pnewfile, siznewfile, readsize, ups);

    fprintf(stderr, "Made successfully\n");
    free(pfile);
    free(pnewfile);
    return 0;
}

static int _upspatch(FILE* file, FILE* ups)
{
    u8 *pfile = NULL, *pups = NULL;
    unsigned int sizfile, sizups;
    int ret;

    if (~ftell(ups))
    {
        sizups = filelength(fileno(ups));
        pups = (u8*)malloc(sizups);
        if (!pups)
        {
            fprintf(stderr, "cannot allocate memory\n");
            return 4;
        }
        fread(pups, 1, sizups, ups);
    }
    else
    { // if not file
        sizups = 0;
        pups = NULL;
        for (;;)
        {
            unsigned int readlen = fread(buf, 1, BUFLEN, stdin);
            if (!readlen)
                break;
            u8* tmp = (u8*)malloc(sizups + readlen);
            if (!tmp)
            {
                if (pups)
                    free(pups);
                fprintf(stderr, "cannot allocate memory\n");
                return 4;
            }
            memcpy(tmp, pups, sizups);
            memcpy(tmp + sizups, buf, readlen);
            if (pups)
                free(pups);
            pups = tmp, tmp = NULL;
            sizups += readlen;
            if (readlen < BUFLEN)
                break;
        }
    }

    fprintf(stderr, "(UPS %u bytes) ", sizups);
    ret = upspatch(NULL, 0, pups, sizups);
    switch (ret)
    {
        case 0:
            sizfile = filelength(fileno(file));
            pfile = (u8*)malloc(sizfile);
            if (!pfile)
            {
                free(pups);
                fprintf(stderr, "cannot allocate memory\n");
                return 4;
            }
            fread(pfile, 1, sizfile, file); // might occur error, but OK
            rewind(file);

            unsigned int crc = 0, patchmode = 0;
            crc = crc32(crc, pfile, sizfile);
            if (crc == read32(pups + (sizups - 12)))
                patchmode = 1;
            if (crc == read32(pups + (sizups - 8)))
                patchmode = 2;
            if (!patchmode)
            {
                free(pfile);
                fprintf(stderr, "incorrect file\n");
                return 4;
            }
            fprintf(stderr, patchmode == 1 ? "forward patch\n" : "reverse patch\n");

            const u8* new;
            u64 sizex = decnum(pups + 4, &new);
            u64 sizey = decnum(new, &new);
            // printf(LLU" "LLU"\n",sizex,sizey);

            // adjust filesize (extend)
            if (patchmode == 1)
            {
                if (sizex < sizey)
                {
                    u8* tmp = (u8*)malloc(sizey);
                    memset(tmp, 0, sizey);
                    if (!tmp)
                    {
                        free(pups);
                        free(pfile);
                        fprintf(stderr, "cannot allocate memory\n");
                        return 4;
                    }
                    memcpy(tmp, pfile, sizex);
                    pfile = tmp;
                }
            }
            else
            {
                if (sizex > sizey)
                {
                    u8* tmp = (u8*)malloc(sizex);
                    memset(tmp, 0, sizex);
                    if (!tmp)
                    {
                        free(pups);
                        free(pfile);
                        fprintf(stderr, "cannot allocate memory\n");
                        return 4;
                    }
                    memcpy(tmp, pfile, sizey);
                    pfile = tmp;
                }
            }
            upspatch(pfile, sizfile, pups, sizups);
            fwrite(pfile, 1, patchmode == 1 ? sizey : sizex, file);
            free(pfile);

            // adjust filesize (truncate)
            if (patchmode == 1)
            {
                if (sizex > sizey)
                {
                    ftruncate(fileno(file), sizey);
                }
            }
            else
            {
                if (sizex < sizey)
                {
                    ftruncate(fileno(file), sizex);
                }
            }
            fprintf(stderr, "Patched successfully\n");
            break;
        case 2:
            fprintf(stderr, "\nups not valid\n");
            break;
        case 1:
            fprintf(stderr, "\nPatch failed (corrupted / truncated)\n");
            break;
    }
    free(pups);
    return ret;
}

// library bootstrap
int xenoups(const int argc, const char** argv)
{
    int flag, ret;
    FILE *file, *newfile = NULL, *ups;

    // startup
    if ((argc < 2 || isatty(fileno(stdin))) && argc < 3)
    {
        fprintf(stderr, "XenoUPS - yet another UPS maker/patcher v2\n"
                        "Usage: xenoups file <ups\n"
                        "       xenoups file ups\n"
                        "       xenoups file newfile >ups\n"
                        "       xenoups file newfile ups\n");
        msleep(500);
        return 1;
    }

    // check running mode
    if (argc == 2)
    {
        file = fopen(argv[1], "r+b");
        if (!file)
            goto failfile;
        ups = stdin;
        flag = 0x00;
    }
    else if (argc == 3)
    {
        if (isatty(fileno(stdout)))
        {
            file = fopen(argv[1], "r+b");
            if (!file)
                goto failfile;
            ups = fopen(argv[2], "rb");
            if (!ups)
            {
                fclose(file);
                goto failfile;
            }
            flag = 0x01;
        }
        else
        {
            file = fopen(argv[1], "rb");
            if (!file)
                goto failfile;
            newfile = fopen(argv[2], "rb");
            if (!newfile)
            {
                fclose(file);
                goto failfile;
            }
            ups = stdout;
            flag = 0x10;
        }
    }
    else
    {
        file = fopen(argv[1], "rb");
        if (!file)
            goto failfile;
        newfile = fopen(argv[2], "rb");
        if (!newfile)
        {
            fclose(file);
            goto failfile;
        }
        ups = fopen(argv[3], "wb");
        if (!ups)
        {
            fclose(file);
            fclose(newfile);
            goto failfile;
        }
        flag = 0x11;
    }

    if (flag & 0x10)
    {
        fprintf(stderr, "Source: %s\nTarget: %s\n", argv[1], argv[2]);
        ret = _upsmake(file, newfile, ups);
        fclose(file);
        fclose(newfile);
        if (flag == 0x11)
            fclose(ups);
        msleep(1000);
        return ret ? ret | 0x20 : 0;
    }
    else
    {
        fprintf(stderr, "File: %s\n", argv[1]);
        ret = _upspatch(file, ups);
        fclose(file);
        if (flag == 0x01)
            fclose(ups);
        msleep(1000);
        return ret ? ret | 0x10 : 0;
    }

failfile:
    fprintf(stderr, "Cannot open file\n");
    msleep(1000);
    return 2;
}
