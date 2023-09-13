#include "../xenobox.h"

static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64_encode(unsigned char k, unsigned char* str, FILE* out)
{
    int i = 0, j = 0, b = 0, I = 0;
    u32 x = 0;
    for (; I < strlen(str); I++)
    {
        x = (x << 8) + (str[I] ^ k);
        i++;
        b += 8;
        while (b >= 6)
            b -= 6, fputc(t[(x >> b) & 0x3f], out), j++;
        // if(j==76){fputc('\n',out);j=0;}
    }
    if (b)
        fputc(t[(x << (6 - b)) & 0x3f], out), j++;
    i %= 3;
    if (i == 2)
        fputs("=", out);
    if (i == 1)
        fputs("==", out);
    // if(j)fputc('\n',out);
    return 0;
}

static int base64_decode(unsigned char k, unsigned char* str, FILE* out)
{
    int b = 0, i = 0;
    u32 x = 0;
    char* p;
    for (; i < strlen(str); i++)
    {
        if (str[i] == '=')
        {
            break;
        }
        if (p = strchr(t, str[i]))
        {
            x = (x << 6) + (p - t);
            b += 6;
            if (b >= 8)
                b -= 8, fputc(((x >> b) & 0xff) ^ k, out);
        }
    }
    while (b >= 8)
        b -= 8, fputc(((x >> b) & 0xff) ^ k, out);
    return 0;
}

int kawencode(const int argc, const char** argv)
{
    unsigned char k = 0xcc;
    int i = 0, fcrypt = 0;
    if (isatty(fileno(stdin)) || isatty(fileno(stdout)))
    {
        fprintf(stderr, "kawencode [key] [-f] <in >out\n");
        return 1;
    }
    if (argc > 1)
        for (k = 0; i < strlen(argv[1]); i++)
            k += argv[1][i];
    if (argc > 2)
        fcrypt = 1;
    for (; myfgets(buf, BUFLEN, stdin);)
    {
        unsigned char* p = buf;
        for (; *p == ' ' || *p == '\t'; p++)
            ;
        if (!strcmp(p, ":crypt"))
        {
            fcrypt = 1;
            continue;
        }
        if (!strcmp(p, ":endcrypt"))
        {
            fcrypt = 0;
            continue;
        }
        if (fcrypt)
        {
            if (argc > 1)
            {
                fwrite("!KAWA0001", 1, 9, stdout);
                fputc(k, stdout);
            }
            else
                fwrite("!KAWA0000", 1, 9, stdout);
            base64_encode(k, p, stdout);
        }
        else
        {
            fwrite(p, 1, strlen(p), stdout);
        }
        /*if(flag)*/ fputc('\n', stdout);
    }
    return 0;
}

int kawdecode(const int argc, const char** argv)
{
    if (isatty(fileno(stdin)) || isatty(fileno(stdout)))
    {
        fprintf(stderr, "kawdecode <in >out\n");
        return 1;
    }
    for (; myfgets(buf, BUFLEN, stdin);)
    {
        if (!strncmp(buf, "!KAWA0000", 9))
        {
            base64_decode(0xcc, buf + 9, stdout);
        }
        else if (!strncmp(buf, "!KAWA0001", 9))
        {
            base64_decode(buf[9], buf + 10, stdout);
        }
        else
        {
            fwrite(buf, 1, strlen(cbuf), stdout);
        }
        /*if(flag)*/ fputc('\n', stdout);
    }
    return 0;
}
