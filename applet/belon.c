#include "../xenobox.h"

// inline int filelength(int fd){
//   int pos=lseek(fd, 0, SEEK_CUR),end=lseek(fd, 0, SEEK_END);
//   lseek(fd, pos, SEEK_SET);return end;
// }

static int bel_compress(const char* filename)
{
    int fs, i, lp, s1l, s2l;
    unsigned char *buf, *p, *s1, *s2, *p1, *p2, a, b, co;
    char bf[260], bf2[260];
    FILE *f1, *f2;

    // load to buf
    f1 = fopen(filename, "rb");
    if (!f1)
    {
        fprintf(stderr, "cannot fopen %s\n", filename);
        return 1;
    }
    fs = filelength(fileno(f1));
    if (fs > 0xffffff)
    {
        fclose(f1);
        fprintf(stderr, "file too large: %s\n", filename);
        return 1;
    }

    // save name
    strcpy(bf, filename);
    p = (u8*)strrchr(bf, '.');
    if (!p)
        p = (u8*)bf + strlen(bf);
    strcpy((char*)p, ".bel");
    f2 = fopen(bf, "wb");
    if (!f2)
    {
        fclose(f1);
        fprintf(stderr, "cannot fopen %s\n", bf);
        return 1;
    }

    buf = (unsigned char*)malloc(fs + 1);
    fread(buf, fs, 1, f1);
    fclose(f1);
    s1 = (unsigned char*)malloc(65536);
    s2 = (unsigned char*)malloc(fs + 1);
    p = buf;
    p1 = s1 + 8;
    p2 = s2;
    lp = 1;
    co = 0;
    s1l = 8;
    s2l = 0;
    b = 0;

    // compress

    // compare against Belc
    // a:fgetc();
    // b:lcomp
    // p1:tmp_dic (s1l:Length)
    // p2:tmp_out (s2l:Length)
    // lp:len1
    // co:len2

    for (i = 0; i < fs; i++)
    {
        a = *(p++);
        if (co)
        {
            if (b != a || co == 255 || i == fs - 1 || !lp)
            {
                if (co == 1)
                {
                    *(p2++) = b;
                    ++s2l;
                    if (++lp == 31)
                    {
                        *(p1++) = 31;
                        ++s1l;
                        lp = 0;
                    }
                }
                else if (co == 2)
                {
                    *(p2++) = b;
                    *(p2++) = b;
                    s2l += 2;
                    lp += 2;
                    if (lp > 30)
                    {
                        *(p1++) = 31;
                        ++s1l;
                        lp -= 31;
                    }
                }
                else
                {
                    if (co < 8)
                    {
                        *(p1++) = lp + co * 32;
                        ++s1l;
                    }
                    else
                    {
                        *(p1++) = lp;
                        *(p1++) = co;
                        s1l += 2;
                    }
                    *(p2++) = b;
                    ++s2l;
                    lp = 1;
                }
                co = 0;
            }
        }
        b = a;
        ++co;
        if (s1l > 65535)
        {
            fclose(f2);
            free(buf);
            free(s1);
            free(s2);
            fprintf(stderr, "Dictionary Buffer Over\n");
            return 2;
        }
    }
    *p1 = 31;
    ++s1l;
    *p2 = b;
    ++s2l; // endmark
    free(buf);

    // make header
    s1[0] = fs & 0x0000ff;
    s1[1] = (fs & 0x00ff00) >> 8;
    s1[2] = (fs & 0xff0000) >> 16;
    s1[3] = s1l & 0x00ff;
    s1[4] = (s1l & 0xff00) >> 8;

    strcpy(bf2, filename);
    p = (u8*)strrchr(bf2, '.');
    if (!p)
        p = (u8*)strrchr(bf2, '/');
    if (!p)
        p = (u8*)strrchr(bf2, '\\');
    if (p)
        p++;
    else
        p = (u8*)bf2;
    strncpy((char*)s1 + 5, (char*)p, 3);

    // save s1,s2
    fwrite(s1, s1l, 1, f2);
    fwrite(s2, s2l, 1, f2);
    fclose(f2);
    free(s1);
    free(s2);
    fprintf(stderr, "compressed to %s(%d bytes)\n", bf, s1l + s2l);
    return 0;
}

static int bel_extract(const char* filename)
{
    int fs1, fs2, i, j, l, lp;
    unsigned char *buf, *buf2, *p, *p1, *p2, a;
    char bf[260];
    FILE *f1, *f2;

    // load to buf1
    f1 = fopen(filename, "rb");
    if (!f1)
    {
        fprintf(stderr, "cannot fopen: %s\n", filename);
        return 1;
    }
    fs1 = filelength(fileno(f1));
    if (fs1 > 0x1100000)
    {
        fclose(f1);
        fprintf(stderr, "file too large: %s\n", filename);
        return 1;
    }
    buf = (unsigned char*)malloc(fs1 + 1);
    fread(buf, fs1, 1, f1);
    fclose(f1);

    // save name
    strcpy(bf, filename);
    p = (u8*)strrchr(bf, '.');
    if (!p)
        p = (u8*)bf + strlen(bf);
    strcpy((char*)p, ".\0\0\0\0");
    for (i = 5; i < 8; i++)
        if (buf[i] == '.' || buf[i] == '/' || buf[i] == '\\')
            buf[i] = '_'; // DTV
    if (!buf[5])
        strcpy((char*)p, ".ext");
    else
        strncpy((char*)p + 1, (char*)buf + 5, 3);
    f2 = fopen(bf, "wb");
    if (!f2)
    {
        free(buf);
        fprintf(stderr, "cannot fopen %s\n", bf);
        return 1;
    }

    // allocate buf2
    fs2 = buf[0] + buf[1] * 256 + buf[2] * 65536;
    p1 = buf + 8;
    p2 = buf + buf[3] + buf[4] * 256;
    buf2 = (unsigned char*)malloc(fs2 + 1);

    // decompress to buf2
    p = buf2;
    i = 0;
    while (i < fs2 - 1)
    {
        lp = *(p1++);
        l = (i + (lp & 31) > fs2 - 1) ? fs2 - i - 1 : lp & 31;
        i += l;
        for (j = 0; j < l; j++)
        {
            *(p++) = *(p2++);
        }
        if (lp != 31)
        {
            if (lp < 32)
                l = *(p1++);
            else
                l = lp / 32;
            a = *(p2 - 1);
            i += l - 1;
            for (j = 0; j < l - 1; j++)
            {
                *(p++) = a;
            }
        }
    }
    *p = *p2;
    free(buf);

    // save buf2
    fwrite(buf2, fs2, 1, f2);
    fclose(f2);
    free(buf2);
    fprintf(stderr, "decompressed to %s(%d bytes)\n", bf, fs2);
    return 0;
}

int belon(const int argc, const char** argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Belon compressor\n"
                        "Usage: belon [c|d] file\n"
                        "\n"
                        "Give 2 args.\n");
        return -1;
    }
    switch (argv[1][0])
    {
        case 'c':
            return bel_compress(argv[2]);
        case 'd':
            return bel_extract(argv[2]);
        default:
            fprintf(stderr, "cmd should be 'c' or 'd'.\n");
            return -1;
    }
}
