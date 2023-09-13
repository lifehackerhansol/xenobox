#ifdef FEOS

// FeOS Helper; wrapper for some functions that FeOS doesn't have.

#include <stdio.h>
#include <string.h>
#include <ctype.h> //tolower

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2

// int isatty(int fd){return 0;} //need to say "redirected", since some of compo requires that...

int open(const char* name, int flags)
{
    char flag[8];
    if ((flags & 3) == O_RDONLY)
        strcpy(flag, "rb");
    else if ((flags & 3) == O_WRONLY)
        strcpy(flag, "wb");
    else if ((flags & 3) == O_RDWR)
        strcpy(flag, "r+b");
    // other flags are ignored currently.

    int ret = (int)fopen(name, flag);
    if (ret == 0)
        return -1;
    return ret;
}

int close(int fd)
{
    return fclose((FILE*)fd);
}

int read(int fd, void* buf, size_t count)
{
    return fread(buf, 1, count, (FILE*)fd);
}

int write(int fd, void* buf, size_t count)
{
    int ret = fwrite(buf, 1, count, (FILE*)fd);
    fflush((FILE*)fd);
    return ret;
}

int lseek(int fd, size_t off, int whence)
{
    int f = fseek((FILE*)fd, off, whence);
    if (f)
        return -1;
    return ftell((FILE*)fd);
}

int access(const char* name, int x)
{
    FILE* f = fopen(name, "rb");
    if (!f)
        return 1;
    fclose(f);
    return 0;
}

int strcasecmp(const char* s1, const char* s2)
{
    while (*s1 != '\0' && tolower(*s1) == tolower(*s2))
        s1++, s2++;
    return tolower(*(unsigned char*)s1) - tolower(*(unsigned char*)s2);
}

int strncasecmp(const char* s1, const char* s2, size_t n)
{
    if (n == 0)
        return 0;
    while (n-- != 0 && tolower(*s1) == tolower(*s2))
    {
        if (n == 0 || *s1 == '\0' || *s2 == '\0')
            break;
        s1++, s2++;
    }
    return tolower(*(unsigned char*)s1) - tolower(*(unsigned char*)s2);
}

// based on WCRT strtol() (C) Ibsen Software under zlib/libpng license.
unsigned long long strtoull(const char* s, char** endp, int base)
{
    const unsigned char* p = (unsigned char*)s;
    unsigned long long res = 0;
    // int negative = 0;

    // skip initial whitespaces
    while (*p && *p <= ' ')
        ++p;

    if (p[0] == 0)
    {
        if (endp)
            *endp = (char*)p;
        return 0;
    }
#if 0
    // check sign
    if ((p[0] == '+') || (p[0] == '-'))
    {
        negative = p[0] == '-' ? 1 : 0;
        ++p;
    }
#endif
    // figure out base if not given
    if (base == 0)
    {
        if (p[0] != '0')
        {
            base = 10;
        }
        else
        {
            if ((p[1] == 'x') || (p[1] == 'X'))
            {
                base = 16;
                p += 2;
            }
            else
            {
                base = 8;
                ++p;
            }
        }
    }
    else
    {

        // 0x/0X is allowed for hex even when base is given
        if ((base == 16) && (p[0] == '0'))
        {
            if ((p[1] == 'x') || (p[1] == 'X'))
            {
                p += 2;
            }
        }
    }

    for (; p[0]; ++p)
    {
        unsigned int val = p[0];

        if (val >= 'a')
            val -= 'a' - 'A';

        if (val >= 'A')
        {
            val = val - 'A' + 10;
        }
        else
        {
            if (val < '0' || val > '9')
                break;
            val = val - '0';
        }

        if (val >= base)
            break;

        res = res * base + val;
    }

    if (endp)
        *endp = (char*)p;

    // return negative ? -((int)res) : (int)res;
    return res;
}

#endif
