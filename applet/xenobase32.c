#include "../xenobox.h"

static const char* t1 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
static const char* t2 = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

static int base32_encode(const char* t)
{ // 5 -> 8
    int i = 0, j = 0, b = 0, c;
    u32 x = 0;
    for (; ~(c = fgetc(stdin));)
    {
        x = (x << 8) + c;
        i++;
        b += 8;
        while (b >= 5)
            b -= 5, fputc(t[(x >> b) & 0x1f], stdout), j++;
        if (j == 72)
        {
            fputc('\n', stdout);
            j = 0;
        }
    }
    if (b)
        fputc(t[(x << (5 - b)) & 0x1f], stdout), j++;
    i %= 5;
    if (i == 4)
        fputs("=", stdout);
    if (i == 3)
        fputs("===", stdout);
    if (i == 2)
        fputs("====", stdout);
    if (i == 1)
        fputs("======", stdout);
    if (j)
        fputc('\n', stdout);
    return 0;
}

static int base32_decode(const char* t)
{
    int b = 0, c;
    u32 x = 0;
    char* p;
    for (; ~(c = fgetc(stdin));)
    {
        if (c == '=')
        {
            break;
        }
        if (between('a', c, 'z'))
            c -= 0x20;
        if (p = strchr(t, c))
        {
            x = (x << 5) + (p - t);
            b += 5;
            if (b >= 8)
                b -= 8, fputc((x >> b) & 0xff, stdout);
        }
    }
    while (b >= 8)
        b -= 8, fputc((x >> b) & 0xff, stdout);
    return 0;
}

int xenobase32(const int argc, const char** argv)
{
    if (argc > 1)
        return base32_decode(t1);
    else
        return base32_encode(t1);
    // return 0;
}

int xenobase32hex(const int argc, const char** argv)
{
    if (argc > 1)
        return base32_decode(t2);
    else
        return base32_encode(t2);
    // return 0;
}
