#include "../xenobox.h"
#include <time.h>

int create(const int argc, const char** argv)
{
    FILE* f;
    unsigned int size;
    if (argc < 3 || !strlen(argv[2]))
    {
        fprintf(stderr, "create file size[kKmM]\n");
        return 1;
    }
    if (!(f = fopen(argv[1], "wb")))
    {
        fprintf(stderr, "cannot open\n");
        return 2;
    }
    memset(buf, 0, BUFLEN);
    size = strtoul(argv[2], NULL, 0);
    if (argv[2][strlen(argv[2]) - 1] == 'k')
        size *= 1000;
    if (argv[2][strlen(argv[2]) - 1] == 'K')
        size *= 1024;
    if (argv[2][strlen(argv[2]) - 1] == 'm')
        size *= 1000000;
    if (argv[2][strlen(argv[2]) - 1] == 'M')
        size *= 1048576;
    while (size)
    {
        fwrite(buf, 1, min(size, BUFLEN), f);
        size -= min(size, BUFLEN);
    }
    fclose(f);
    return 0;
}

int createff(const int argc, const char** argv)
{
    FILE* f;
    unsigned int size;
    if (argc < 3 || !strlen(argv[2]))
    {
        fprintf(stderr, "createff file size[kKmM]\n");
        return 1;
    }
    if (!(f = fopen(argv[1], "wb")))
    {
        fprintf(stderr, "cannot open\n");
        return 2;
    }
    memset(buf, 0xff, BUFLEN);
    size = strtoul(argv[2], NULL, 0);
    if (argv[2][strlen(argv[2]) - 1] == 'k')
        size *= 1000;
    if (argv[2][strlen(argv[2]) - 1] == 'K')
        size *= 1024;
    if (argv[2][strlen(argv[2]) - 1] == 'm')
        size *= 1000000;
    if (argv[2][strlen(argv[2]) - 1] == 'M')
        size *= 1048576;
    while (size)
    {
        fwrite(buf, 1, min(size, BUFLEN), f);
        size -= min(size, BUFLEN);
    }
    fclose(f);
    return 0;
}

int _truncate(const int argc, const char** argv)
{
    FILE* f;
    unsigned int size;
    if (argc < 3 || !strlen(argv[2]))
    {
        fprintf(stderr, "ftruncate file size[kKmM]\n");
        return 1;
    }
    if (!(f = fopen(argv[1], "r+b")))
    {
        fprintf(stderr, "cannot open\n");
        return 2;
    }
    size = strtoul(argv[2], NULL, 0);
    if (argv[2][strlen(argv[2]) - 1] == 'k')
        size *= 1000;
    if (argv[2][strlen(argv[2]) - 1] == 'K')
        size *= 1024;
    if (argv[2][strlen(argv[2]) - 1] == 'm')
        size *= 1000000;
    if (argv[2][strlen(argv[2]) - 1] == 'M')
        size *= 1048576;
    ftruncate(fileno(f), size);
    fclose(f);
    return 0;
}

int _extend(const int argc, const char** argv)
{
    FILE* f;
    unsigned int size, fsize;
    if (argc < 3 || !strlen(argv[2]))
    {
        fprintf(stderr, "fextend file size[kKmM]\n");
        return 1;
    }
    if (!(f = fopen(argv[1], "r+b")))
    {
        fprintf(stderr, "cannot open\n");
        return 2;
    }
    fsize = filelength(fileno(f));
    size = strtoul(argv[2], NULL, 0);
    if (argv[2][strlen(argv[2]) - 1] == 'k')
        size *= 1000;
    if (argv[2][strlen(argv[2]) - 1] == 'K')
        size *= 1024;
    if (argv[2][strlen(argv[2]) - 1] == 'm')
        size *= 1000000;
    if (argv[2][strlen(argv[2]) - 1] == 'M')
        size *= 1048576;
    if (fsize > size)
    {
        fclose(f);
        fprintf(stderr, "cannot extend. ftruncate instead\n");
        return 3;
    }

    memset(buf, 0, BUFLEN);
    fseek(f, fsize, SEEK_SET);
    size -= fsize;
    while (size)
        fwrite(buf, 1, size < BUFLEN ? size : BUFLEN, f), size -= (size < BUFLEN ? size : BUFLEN);
    fclose(f);
    return 0;
}

int _cutdown(const int argc, const char** argv)
{
    FILE* f;
    unsigned int /*fsize,*/ start, end, size;
    if (argc < 3 || !strlen(argv[2]))
    {
        fprintf(stderr, "fcutdown in[stdin:-] start[kKmM] end[kKmM][lLoO] > out\nl=Length o=offset\n");
        return 1;
    }
    if (!strcmp(argv[1], "-"))
        f = stdin;
    else
        f = fopen(argv[1], "rb");
    if (!f)
    {
        fprintf(stderr, "cannot open\n");
        return 2;
    }
    // fsize=filelength(fileno(f));

    start = strtoul(argv[2], NULL, 0);
    if (argv[2][strlen(argv[2]) - 1] == 'k')
        start *= 1000;
    if (argv[2][strlen(argv[2]) - 1] == 'K')
        start *= 1024;
    if (argv[2][strlen(argv[2]) - 1] == 'm')
        start *= 1000000;
    if (argv[2][strlen(argv[2]) - 1] == 'M')
        start *= 1048576;

    if (argc > 3 && strlen(argv[3]))
    {
        end = strtoul(argv[3], NULL, 0);
        int length = 0;
        char unit = argv[3][strlen(argv[3]) - 1];
        if (unit == 'l' || unit == 'L' || unit == 'o' || unit == 'O')
        {
            if (unit == 'l' || unit == 'L')
                length = 1;
            if (strlen(argv[3]) < 2)
                goto cutdown_end_is_fsize;
            unit = argv[3][strlen(argv[3]) - 2];
        }
        if (unit == 'k')
            end *= 1000;
        if (unit == 'K')
            end *= 1024;
        if (unit == 'm')
            end *= 1000000;
        if (unit == 'M')
            end *= 1048576;
        if (length)
            end += start;
    }
    else
    {
    cutdown_end_is_fsize:
        end = -1; // fsize;
    }
    if (start > end)
    {
        fclose(f);
        fprintf(stderr, "start offset exceeds end offset. %08x > %08x\n", start, end);
        return 3;
    }
    // if exceeds, output just truncated...
    // if(end>fsize){fclose(f);fprintf(stderr,"end offset exceeds fsize. %08x > %08x\n",end,fsize);return 3;}

    memset(buf, 0, BUFLEN);
    size = end - start; // !!!
    // fseek(f,start,SEEK_SET);
    unsigned int readlen;
    for (; start > 0; start -= BUFLEN)
    {
        readlen = fread(buf, 1, start < BUFLEN ? start : BUFLEN, f);
        if (readlen < BUFLEN)
            break;
    }
    for (; size > 0; size -= BUFLEN)
    {
        readlen = fread(buf, 1, size < BUFLEN ? size : BUFLEN, f);
        fwrite(buf, 1, readlen, stdout);
        // if(readlen<(size<BUFLEN?size:BUFLEN)||size<BUFLEN)break;
        if (readlen < BUFLEN)
            break;
    }
    if (f != stdin)
        fclose(f);
    return 0;
}

int _msleep(const int argc, const char** argv)
{
    int msec;
    if (argc < 2)
    {
        fprintf(stderr, "msleep [arg](ms)\n");
        return 1;
    }
    msec = strtol(argv[1], 0, 0);
    msleep(msec);
    return 0;
}

int _xenofrob(FILE* in, FILE* out, int key)
{
    int i, readlen;
    for (; readlen = fread(buf, 1, BUFLEN, in); fwrite(buf, 1, readlen, out))
        for (i = 0; i < readlen; i++)
            buf[i] ^= key;
    return 0;
}
int xenofrob(const int argc, const char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "xenofrob [key]\nkey: memfrob is 42, nsc is 0x84.\n");
        return 1;
    }
    return _xenofrob(stdin, stdout, strtol(argv[1], NULL, 0));
}
int nsc(const int argc, const char** argv)
{
    return _xenofrob(stdin, stdout, 0x84);
}

static int zeller(int Y, int M, int D)
{
    if (++M < 4)
        Y -= 1, M += 12;
    int y = Y / 100, z = Y % 100;
    return (5 * y + z + y / 4 + z / 4 + 13 * M / 5 + D - 1) % 7;
}
static char* W[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
int xenozeller(const int argc, const char** argv)
{
    int i = 1;
    unsigned int n, y, m, d;
    if (argc < 2)
    {
        fprintf(stderr, "xenozeller YYYYMMDD...\n");
        return 1;
    }
    for (; i < argc; i++)
    {
        n = strtoul(argv[i], NULL, 0);
        y = n / 10000, m = n / 100 % 100, d = n % 100;
        printf("%d/%02d/%02d: ", y, m, d);
        if (between(1, m, 12) && between(1, d, 31))
            printf("%s\n", W[zeller(y, m, d)]);
        else
            printf("illegal date\n");
    }
    return 0;
}

static int leap(const int y)
{
    if (y % 400 == 0)
        return 1;
    if (y % 100 == 0)
        return 0;
    return y % 4 == 0;
}

static char* M2[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static char* M2_long[] = { "January", "February", "March",     "April",   "May",      "June",
                           "July",    "August",   "September", "October", "November", "December" };
static char* W2[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
int xenodate(const int argc, const char** argv)
{
    time_t timer;
    struct tm* pt;
    time(&timer);
    pt = localtime(&timer);
#if defined(FEOS)
    printf("%d %s %d %s %02d:%02d:%02d\n", pt->tm_year + 1900, M2[pt->tm_mon], pt->tm_mday, W2[pt->tm_wday],
           pt->tm_hour, pt->tm_min, pt->tm_sec);
#else
    printf("%d %s %d %s %02d:%02d:%02d %s\n", pt->tm_year + 1900, M2[pt->tm_mon], pt->tm_mday, W2[pt->tm_wday],
           pt->tm_hour, pt->tm_min, pt->tm_sec, tzname[1]);
#endif
    return 0;
}

int xenoexcelbase(const int argc, const char** argv)
{
    int i = 1, j, k, l;
    if (argc < 2)
    {
        fprintf(stderr, "xenoexcelbase ALNUM...\n");
        return 1;
    }
    for (; i < argc; i++)
    {
        printf("%s -> ", argv[i]);
        l = strlen(argv[i]);
        for (j = 0; j < l; j++)
            if (!between('0', argv[i][j], '9'))
                break;
        for (k = 0; k < l; k++)
            if (!(between('a', argv[i][k], 'z') || between('A', argv[i][k], 'Z')))
                break;
        if (j < l && k < l)
        {
            printf("illegal format.\n");
            continue;
        }
        if (j == l)
        { // NUM to AL
            unsigned long long int r = strtoull(argv[i], NULL, 0);
            int flg = 0;
            char xxx[30];
            for (j = 0; r; j++, r /= 26)
            {
                xxx[j] = r % 26 + 'A' - 1 - flg;
                flg = 0;
                if (xxx[j] == '@')
                    xxx[j] = 'Z', flg = 1;
            }
            if (flg)
                xxx[j] = 0, j--;
            for (j--; j >= 0; j--)
            {
                putchar(xxx[j]);
            }
            putchar('\n');
        }
        if (k == l)
        { // AL to NUM
            unsigned long long int r = 0;
            for (k = 0; k < l; k++)
            {
                int c = argv[i][k];
                if (between('a', argv[i][k], 'z'))
                    c -= 0x20;
                r *= 26;
                r += c - 'A' + 1;
            }
            printf(LLU "\n", r);
        }
    }
    return 0;
}

int unsnowflake(const int argc, const char** argv)
{
    int i = 1;
    if (argc < 2)
    {
        fprintf(stderr, "unsnowflake status_id...\n");
        return 1;
    }
    for (; i < argc; i++)
    {
        unsigned long long int x = strtoull(argv[i], NULL, 0);
        unsigned int sequence = x & 0xfff;
        unsigned int worker = (x >> 12) & 0x1f;
        unsigned int center = (x >> 17) & 0x1f;
        unsigned long long int twepoch = (x >> 22) + 1288834974657ULL;
        unsigned int twepoch_ms = twepoch % 1000;
        twepoch /= 1000;
        time_t timer = (time_t)twepoch;
        struct tm* pt;
        pt = localtime(&timer);
        printf("%04d/%02d/%02d %02d:%02d:%02d.%03u,datacenter=%u,worker=%u,sequence=%u\n", pt->tm_year + 1900,
               pt->tm_mon + 1, pt->tm_mday, pt->tm_hour, pt->tm_min, pt->tm_sec, twepoch_ms, center, worker, sequence);
    }
    return 0;
}

// argments is meant to be used for injection. not for integrating to xenobox.
// int xenoargs(const int argc, const char **argv);

static int _xenoalloc(const int n)
{
    int r = 0;
    void* p = malloc(1024 * 1024);
    if (!p)
        return n;
    if (n > 1)
        r = _xenoalloc(n - 1);
    free(p);
    return r;
}

int xenoalloc(const int argc, const char** argv)
{
    if (argc < 2)
        goto xenoalloc_err;
    int n = strtol(argv[1], NULL, 0);
    if (n < 1)
        goto xenoalloc_err;
    int r = _xenoalloc(n);
    if (r)
        fprintf(stderr, "failed to alloc %dMB.\n", r);
    return r;
xenoalloc_err:
    fprintf(stderr, "xenoalloc n[MB]\n");
    return -1;
}

int xenohead(const int argc, const char** argv)
{
    if (isatty(fileno(stdin)))
    {
        fprintf(stderr, "xenohead [lines=10] <in [>out]\nNote: use fcutdown - 0 N[bytes] for binary files.\n");
        return 1;
    }
    int lines = argc >= 2 ? strtol(argv[1], NULL, 0) : 10, i = 0;
    for (; i < lines; i++)
    {
        if (!fgets(buf, BUFLEN, stdin))
            break;
        fwrite(buf, 1, strlen(buf), stdout);
    }
    return 0;
}

static int days_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
int xenocal(const int argc, const char** argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "xenocal month year\n");
        return 1;
    }
    int month = strtol(argv[1], NULL, 0);
    int year = strtol(argv[2], NULL, 0);
    if (leap(year))
        days_month[1] = 29;
    int day = zeller(year, month, 1);
    if (month < 1 || 12 < month)
    {
        fprintf(stderr, "month is 1-12\n");
        return 1;
    }
    month -= 1;
    int i = 0;
    printf("%s %d\nSu Mo Tu We Th Fr Sa\n", M2_long[month], year);
    for (; i < day; i++)
    {
        if (i % 7)
            putchar(' ');
        printf("  ");
    }
    for (; i < day + days_month[month]; i++)
    {
        if (i % 7)
            putchar(' ');
        printf("%2d", i - day + 1);
        if (i % 7 == 6)
            putchar('\n');
    }
    if (i % 7)
        putchar('\n');
    return 0;
}
