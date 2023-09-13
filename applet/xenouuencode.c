#include "../xenobox.h"

static const char* t = "`!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";

static int uuencode_encode()
{
    int i = 0, j = 0, b = 0, c;
    u32 x = 0;
    for (; ~(c = fgetc(stdin));)
    {
        x = (x << 8) + c;
        i++;
        b += 8;
        while (b >= 6)
            b -= 6, fputc(t[(x >> b) & 0x3f], stdout), j++;
        if (j == 76)
        {
            fputc('\n', stdout);
            j = 0;
        }
    }
    if (b)
        fputc(t[(x << (6 - b)) & 0x3f], stdout), j++;
    // i%=3;
    // if(i==2)fputs("=",stdout);
    // if(i==1)fputs("==",stdout);
    if (j)
        fputc('\n', stdout);
    return 0;
}

static int uuencode_decode()
{
    int b = 0, c;
    u32 x = 0;
    char* p;
    for (; ~(c = fgetc(stdin));)
    {
        if (c == ' ')
            c = '`';
        if (between('a', c, 'z'))
            c -= 0x20;
        if (p = strchr(t, c))
        {
            x = (x << 6) + (p - t);
            b += 6;
            if (b >= 8)
                b -= 8, fputc((x >> b) & 0xff, stdout);
        }
    }
    while (b >= 8)
        b -= 8, fputc((x >> b) & 0xff, stdout);
    return 0;
}

int xenouuencode(const int argc, const char** argv)
{
    if (argc > 1)
        return uuencode_decode();
    else
        return uuencode_encode();
    // return 0;
}
