#include "../xenobox.h"

static int parse(const char* a, unsigned char* p, unsigned int* l)
{
    int i = 0, t, s;
    for (*l = 0; i < strlen(a); i++)
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
                /*
                            case ':':
                                if(*p)return 10;
                                i++;*l=0;
                                for(;a[i];i++){
                                    if('0'<=a[i]&&a[i]<='9')t=a[i]-'0';
                                    else if('A'<=a[i]&&a[i]<='F')t=a[i]-'A'+10;
                                    else if('a'<=a[i]&&a[i]<='f')t=a[i]-'a'+10;
                                    else return 11;
                                    *l=(*l)*16+t;
                                }
                                return 0;
                */
            default:
                return 4;
        }
    }
    return 0;
}

// (/:)9ab /x1a/x2b
int sucnv2ips(const int argc, const char** argv)
{
    char *addr, *arg;
    unsigned char x[2048], b[3];
    unsigned int z = 0, t;
    if (isatty(fileno(stdin)) || isatty(fileno(stdout)))
    {
        fprintf(stderr, "sucnv2ips <sucnv >ips\nsucnv format: 9ab /x1a/x2b\n");
        return -1;
    }
    fwrite("PATCH", 1, 5, stdout);
    for (; myfgets(cbuf, 2048, stdin);)
    {
        if (!*buf)
            goto next;
        arg = cbuf + strlen(cbuf) - 1;
    striplast:
        for (; *arg == ' '; arg--)
            if (arg == cbuf)
            {
                fprintf(stderr, "format error: %s\n", cbuf);
                goto next;
            }
        arg[1] = 0; // strip trailing space
        for (; *arg != ' '; arg--)
            if (arg == cbuf)
            {
                fprintf(stderr, "format error: %s\n", cbuf);
                goto next;
            }
        if (arg[1] == '>')
            goto striplast;
        arg++;

        for (addr = arg - 1; *addr == ' '; addr--)
            if (addr == cbuf)
            {
                fprintf(stderr, "format error: %s\n", cbuf);
                goto next;
            }
        addr[1] = 0; // strip trailing space
        for (; *addr != ' '; addr--)
            if (addr == cbuf)
            {
                fprintf(stderr, "format error: %s\n", cbuf);
                goto next;
            }
        addr++;
        for (z = 0; *addr; addr++)
        {
            t = 0;
            if ('0' <= *addr && *addr <= '9')
                t = *addr - '0';
            else if ('A' <= *addr && *addr <= 'F')
                t = *addr - 'A' + 10;
            else if ('a' <= *addr && *addr <= 'f')
                t = *addr - 'a' + 10;
            // else ignore.
            z = z * 16 + t;
        }

        if (parse(arg, x, &t))
        {
            fprintf(stderr, "cannot parse %s\n", arg);
            goto next;
        }
        write24be(b, z);
        fwrite(b, 1, 3, stdout);
        write16be(b, t);
        fwrite(b, 1, 2, stdout);
        fwrite(x, 1, t, stdout);
    next:;
    }
    fwrite("EOF", 1, 3, stdout);
    return 0;
}
