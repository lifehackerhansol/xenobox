#include "../xenobox.h"

static unsigned int address;
static int parse(const char* a, unsigned char* p, unsigned int* l)
{
    int i = 0, t, s;
    for (*p = *l = 0; i < strlen(a); i++)
    {
        if (a[i] != '/')
        {
            p[(*l)++] = a[i];
            continue;
        }
        i++;
        if (i == strlen(a))
            return 1;
        switch (a[i])
        {
            case 'n':
            case 'N':
                p[(*l)++] = '\n';
                break;
            case 'r':
            case 'R':
                p[(*l)++] = '\r';
                break;
            case 's':
            case 'S':
                p[(*l)++] = ' ';
                break;
            case 't':
            case 'T':
                p[(*l)++] = '\t';
                break;
            case 'm':
            case 'M':
                p[(*l)++] = '>';
                break;
            case 'h':
            case 'H':
                p[(*l)++] = '<';
                break;
            case 'q':
            case 'Q':
                p[(*l)++] = '?';
                break;
            case 'a':
            case 'A':
                p[(*l)++] = '*';
                break;
            case 'd':
            case 'D':
                p[(*l)++] = '$';
                break;
            case '=':
                p[(*l)++] = '~';
                break;
            case '/':
                p[(*l)++] = '/';
                break;
            case 'p':
            case 'P':
                p[(*l)++] = '|';
                break;
            case '0':
                p[(*l)++] = '\0';
                break;
            case 'x':
            case 'X':
                i++;
                if (strlen(a) - i < 2)
                    return 2;
                if ('0' <= a[i] && a[i] <= '9')
                    t = a[i] - '0';
                else if ('A' <= a[i] && a[i] <= 'F')
                    t = a[i] - 'A' + 10;
                else if ('a' <= a[i] && a[i] <= 'f')
                    t = a[i] - 'a' + 10;
                else
                    return 3;
                i++;
                if ('0' <= a[i] && a[i] <= '9')
                    s = a[i] - '0';
                else if ('A' <= a[i] && a[i] <= 'F')
                    s = a[i] - 'A' + 10;
                else if ('a' <= a[i] && a[i] <= 'f')
                    s = a[i] - 'a' + 10;
                else
                    return 3;
                p[(*l)++] = (t << 4) | s;
                break;
            case ':':
                if (*p)
                    return 10;
                i++;
                address = 0;
                for (; a[i]; i++)
                {
                    if ('0' <= a[i] && a[i] <= '9')
                        t = a[i] - '0';
                    else if ('A' <= a[i] && a[i] <= 'F')
                        t = a[i] - 'A' + 10;
                    else if ('a' <= a[i] && a[i] <= 'f')
                        t = a[i] - 'a' + 10;
                    else
                        return 11;
                    address = (address << 4) + t;
                }
                return 0;
            default:
                return 4;
        }
    }
    return 0;
}

int _memcmp(const void* p, const void* q, const int s)
{
    int i = 0, x;
    for (; i < s; i++)
        if (x = ((char*)q)[i] - ((char*)p)[i])
            return x;
    return 0;
}

int binreplace(const int argc, const char** argv)
{
    unsigned char *p, *b, *a;
    unsigned size, sb, sa;
    FILE* f;

    // startup
    if (argc < 4)
    {
        fprintf(stderr, "binreplace file before after\n"
                        "/n = LF     /r = CR\n"
                        "/s = Space  /t = Tab\n"
                        "/m = >      /h = <\n"
                        "/q = ?      /a = *\n"
                        "/d = $      /= = ~\n"
                        "// = /      /p = |\n"
                        "/0 = \\0\n"
                        "/x** = Hexadecimal form\n"
                        "/:** = Replace address in hex\n"
                        "Example:\n"
                        "[Lock DLDI]\n"
                        "binreplace file /xed/xa5/x8d/xbf/sChishm/0/x01 /xff/xa5/x8d/xbf/sChishm/0/x01\n\n"
                        "Limitation: length of parse(before) and parse(after) have to be the same.\n\n");
        return 1;
    }

    b = (unsigned char*)malloc(strlen(argv[2]) + 1);
    if (!b)
    {
        fprintf(stderr, "cannot alloc memory\n");
        return 1;
    }
    a = (unsigned char*)malloc(strlen(argv[3]) + 1);
    if (!a)
    {
        fprintf(stderr, "cannot alloc memory\n");
        free(b);
        return 1;
    }
    *b = *a = 0;
    if (parse(argv[2], b, &sb) || parse(argv[3], a, &sa))
    {
        fprintf(stderr, "parse error\n");
        free(b);
        free(a);
        return 2;
    }
    if (!sb)
    {
        if (!sa)
        {
            fprintf(stderr, "both args length 0\n");
            free(b);
            free(a);
            return 11;
        }
        // treats sb as seekpoint
    }
    else if (sb != sa)
    {
        fprintf(stderr, "length of parse(before) and parse(after) aren't the same\n");
        free(b);
        free(a);
        return 3;
    }

    f = fopen(argv[1], "r+b");
    if (!f)
    {
        fprintf(stderr, "cannot open file\n");
        free(b);
        free(a);
        return 4;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size > 0x2000000)
    {
        fprintf(stderr, "too big (max 32MB)\n");
        fclose(f);
        free(b);
        free(a);
        return 5;
    }
    p = (unsigned char*)malloc(size);
    if (!p)
    {
        fprintf(stderr, "cannot alloc memory\n");
        fclose(f);
        free(b);
        free(a);
        return 6;
    }
    fread(p, 1, size, f);
    rewind(f);

    if (sb)
    {
        int i = 0;
        for (; i < size - sb; i++)
            if (!_memcmp(p + i, b, sb))
            {
                fprintf(stderr, "replaced 0x%08x\n", i);
                memcpy(p + i, a, sa);
                i += sa;
            }
    }
    else
    {
        memcpy(p + address, a, sa);
    }

    fwrite(p, 1, size, f);
    fclose(f);
    free(p);
    free(b);
    free(a);
    return 0;
}
