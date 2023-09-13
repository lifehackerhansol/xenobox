#include "../xenobox.h"

static void ppfwrite(const u8* p, const unsigned int address, const unsigned int _size, FILE* ppf)
{
    u8 head[5];
    unsigned int size = _size, patchofs = 0;
    while (size)
    {
        // fprintf(stderr,"write 0x%06x %dbytes\n",address+patchofs,min(size,255));
        write32(head, address + patchofs);
        head[4] = min(size, 255);
        fwrite(head, 1, 5, ppf);
        fwrite(p + address + patchofs, 1, min(size, 255), ppf);
        patchofs += min(size, 255);
        size -= min(size, 255);
    }
}

static int ppfmake(const u8* pfile, const unsigned int sizfile, const u8* pnewfile, const unsigned int siznewfile,
                   FILE* ppf)
{
    fwrite("PPF10\0", 1, 6, ppf);
    memset(buf, 0, 50);
    fwrite(buf, 1, 50, ppf);

    unsigned int i = 0, address, final;
    for (; i < sizfile; i++)
    {
        if (pfile[i] != pnewfile[i])
        {
            address = final = i;
            for (; memcmp_fast(pfile + final, pnewfile + final, min(5, sizfile - final)); final++)
                i++;
            ppfwrite(pnewfile, address, final - address, ppf);
        }
    }
    fflush(ppf);
    return 0;
}

static int ppfpatch(u8* pfile, unsigned int sizfile, const u8* _pppf, const unsigned int _sizppf)
{ // pass pfile=NULL to get sizfile.
    if (_sizppf < 56 || memcmp(_pppf, "PPF", 3))
        return 2;
    char mode = _pppf[3] - '0';
    if (mode < 1 || 3 < mode)
        return 2;

    // header check
    const u8* pppf = _pppf;
    unsigned int sizppf = _sizppf;
    u8 undo = 0;
    int isReverse = -1;
    if (mode == 1)
    {
        pppf += 56;
        sizppf -= 56;
    }
    if (mode == 2)
    {
        if (_sizppf < 56 + 4 + 1024)
            return 2;
        if (sizfile && sizfile != read32(_pppf + 56))
            return 2;
        if (memcmp(pfile + 0x9320, _pppf + 60, 1024))
            return 2;
        pppf += 56 + 4 + 1024;
        sizppf -= 56 + 4 + 1024;
        if (!memcmp(pppf + sizppf - 4 - 16, "@END_FILE_ID.DIZ", 16))
        {
            u32 len = read32(pppf + sizppf - 4);
            pppf += 18 + len + 16;
            sizppf -= 18 + len + 16;
        }
    }
    if (mode == 3)
    {
        if (_sizppf < 56 + 4 + 1024)
            return 2;
        u8 imagetype = _pppf[56];
        u8 blockcheck = _pppf[57];
        undo = _pppf[58];
        pppf += 56 + 4;
        sizppf -= 56 + 4;
        if (blockcheck)
        {
            if (memcmp(pfile + (imagetype == 0 ? 0x9320 : 0x80A0), _pppf + 60, 1024))
                return 2;
            pppf += 1024;
            sizppf -= 1024;
        }
        if (!memcmp(pppf + sizppf - 4 - 16, "@END_FILE_ID.DIZ", 16))
        {
            u32 len = read32(pppf + sizppf - 4);
            pppf += 18 + len + 16;
            sizppf -= 18 + len + 16;
        }
    }

    const u8* pppfend = pppf + sizppf;
    // now, parse body
    for (;;)
    {
        if (pppf == pppfend)
            return 0;
        u64 offset = 0;
        u8 length = 0;
        if (mode == 1 || mode == 2)
        {
            if (pppf + 5 > pppfend)
                return 1;
            offset = read32(pppf);
            length = pppf[4];
            pppf += 5;
        }
        if (mode == 3)
        {
            if (pppf + 9 > pppfend)
                return 1;
            offset = read64(pppf);
            length = pppf[8];
            pppf += 9;
        }

        if (pppf + length * (undo ? 2 : 1) > pppfend)
            return 1;
        if (pfile)
        {
            if (undo)
            {
                if (isReverse == -1)
                {
                    fprintf(stderr, "reversible: ");
                    if (!memcmp(pfile + offset, pppf + length, length))
                    {
                        isReverse = 0;
                        fprintf(stderr, "forward patch.\n");
                    }
                    if (!memcmp(pfile + offset, pppf, length))
                    {
                        isReverse = 1;
                        fprintf(stderr, "reverse patch.\n");
                    }
                    if (isReverse == -1)
                    {
                        fprintf(stderr, "integrity check failed.\n");
                        return 1;
                    }
                }
                if (isReverse == 0)
                {
                    if (memcmp(pfile + offset, pppf + length, length))
                    {
                        fprintf(stderr, "integrity check failed.\n");
                        return 1;
                    }
                    memcpy(pfile + offset, pppf, length);
                }
                if (isReverse == 1)
                {
                    if (memcmp(pfile + offset, pppf, length))
                    {
                        fprintf(stderr, "integrity check failed.\n");
                        return 1;
                    }
                    memcpy(pfile + offset, pppf + length, length);
                }
            }
            memcpy(pfile + offset, pppf, length);
        }
        pppf += length * (undo ? 2 : 1);
    }
    return 0; ///
}

// bootstrap
static int _ppfmake(FILE* file, FILE* newfile, FILE* ppf)
{
    u8 *pfile = NULL, *pnewfile = NULL;
    unsigned int sizfile, siznewfile;
    // int ret;

    sizfile = filelength(fileno(file));
    siznewfile = filelength(fileno(newfile));
    if (sizfile != siznewfile)
    {
        fprintf(stderr, "file size not the same\n");
        return 2;
    }

    pfile = (u8*)malloc(sizfile);
    pnewfile = (u8*)malloc(siznewfile);
    if (!pfile || !pnewfile)
    {
        if (pfile)
            free(pfile);
        if (pnewfile)
            free(pnewfile);
        fprintf(stderr, "cannot allocate memory\n");
        return 2;
    }
    fread(pfile, 1, sizfile, file);
    fread(pnewfile, 1, siznewfile, newfile);

    // fprintf(stderr,"(%u bytes / %u bytes)\n",sizfile,siznewfile);
    ppfmake(pfile, sizfile, pnewfile, siznewfile, ppf);

    fprintf(stderr, "Made successfully\n");
    free(pfile);
    free(pnewfile);
    return 0;
}

static int _ppfpatch(FILE* file, FILE* ppf)
{
    u8 *pfile = NULL, *pppf = NULL;
    unsigned int sizfile, sizppf;
    int ret;

    if (~ftell(ppf))
    {
        sizppf = filelength(fileno(ppf));
        pppf = (u8*)malloc(sizppf);
        if (!pppf)
        {
            fprintf(stderr, "cannot allocate memory\n");
            return 4;
        }
        fread(pppf, 1, sizppf, ppf);
    }
    else
    { // if not file
        sizppf = 0;
        pppf = NULL;
        for (;;)
        {
            unsigned int readlen = fread(buf, 1, BUFLEN, stdin);
            if (!readlen)
                break;
            u8* tmp = (u8*)malloc(sizppf + readlen);
            if (!tmp)
            {
                if (pppf)
                    free(pppf);
                fprintf(stderr, "cannot allocate memory\n");
                return 4;
            }
            memcpy(tmp, pppf, sizppf);
            memcpy(tmp + sizppf, buf, readlen);
            if (pppf)
                free(pppf);
            pppf = tmp, tmp = NULL;
            sizppf += readlen;
            if (readlen < BUFLEN)
                break;
        }
    }

    fprintf(stderr, "(PPF %u bytes)\n", sizppf);
    ret = ppfpatch(NULL, 0, pppf, sizppf);
    switch (ret)
    {
        case 0:
            sizfile = filelength(fileno(file));
            pfile = (u8*)malloc(sizfile);
            if (!pfile)
            {
                free(pppf);
                fprintf(stderr, "cannot allocate memory\n");
                return 4;
            }
            fread(pfile, 1, sizfile, file); // might occur error, but OK
            rewind(file);

            if (!ppfpatch(pfile, sizfile, pppf, sizppf))
                fwrite(pfile, 1, sizfile, file);
            free(pfile);
            fprintf(stderr, "Patched successfully\n");
            break;
        case 2:
            fprintf(stderr, "\nppf not valid\n");
            break;
        case 1:
            fprintf(stderr, "\nPatch failed (corrupted / truncated)\n");
            break;
    }
    free(pppf);
    return ret;
}

// library bootstrap
int xenoppf(const int argc, const char** argv)
{
    int flag, ret;
    FILE *file, *newfile = NULL, *ppf;

    // startup
    if ((argc < 2 || isatty(fileno(stdin))) && argc < 3)
    {
        fprintf(stderr, "XenoPPF - yet another ppf maker/patcher v1\n"
                        "Currently you can make PPF1.\n"
                        "Usage: xenoppf file <ppf\n"
                        "       xenoppf file ppf\n"
                        "       xenoppf file newfile >ppf\n"
                        "       xenoppf file newfile ppf\n");
        msleep(500);
        return 1;
    }

    // check running mode
    if (argc == 2)
    {
        file = fopen(argv[1], "r+b");
        if (!file)
            goto failfile;
        ppf = stdin;
        flag = 0x00;
    }
    else if (argc == 3)
    {
        if (isatty(fileno(stdout)))
        {
            file = fopen(argv[1], "r+b");
            if (!file)
                goto failfile;
            ppf = fopen(argv[2], "rb");
            if (!ppf)
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
            ppf = stdout;
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
        ppf = fopen(argv[3], "wb");
        if (!ppf)
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
        ret = _ppfmake(file, newfile, ppf);
        fclose(file);
        fclose(newfile);
        if (flag == 0x11)
            fclose(ppf);
        msleep(1000);
        return ret ? ret | 0x20 : 0;
    }
    else
    {
        fprintf(stderr, "File: %s\n", argv[1]);
        ret = _ppfpatch(file, ppf);
        fclose(file);
        if (flag == 0x01)
            fclose(ppf);
        msleep(1000);
        return ret ? ret | 0x10 : 0;
    }

failfile:
    fprintf(stderr, "Cannot open file\n");
    msleep(1000);
    return 2;
}
