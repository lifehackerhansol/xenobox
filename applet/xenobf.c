#include "../xenobox.h"

static unsigned char* a;
static void execute(char* p, int braincrash)
{
    char *x = p, c;
    int i = 0, marker = 0;
    while (c = *(x + i))
    {
        i++;
        switch (c)
        {
            case '>':
                a++;
                break;
            case '<':
                a--;
                break;
            case '+':
                (*a)++;
                break;
            case '-':
                (*a)--;
                break;
            case '.':
                putchar(*a);
                break;
            case ',':
                *a = getchar();
                break;
            case '[':
            {
                if (*a)
                    execute(x + i, 0);
                for (marker = 1; marker; i++)
                {
                    if (*(x + i) == '[')
                        marker++;
                    if (*(x + i) == ']')
                        marker--;
                }
            }
            break;
            case ']':
                if (*a)
                    i = 0;
                else
                    return;
                break;

            // braincrash
            case '|':
                a++;
                *a |= *(a - 1);
            case '&':
                a++;
                *a &= *(a - 1);
            case '~':
                *a = ~(*a);
            case '^':
                a++;
                *a ^= *(a - 1);

            default:
            {
                fprintf(stderr, "internal error: operation %c\n", c);
                return;
            }
        }
    }
    if (braincrash)
        for (; *a; a++)
            putchar(*a);
}

int xenobf(const int argc, const char** argv)
{
    int c, i = 0, l;
    unsigned char* da;
    char *p, *x;
    FILE* f;

    if (argc != 2)
    {
        fprintf(stderr, "Brainfuck Interpreter\n"
                        "xenobf _.bf\n"
                        "xenobf :_.bf (braincrash mode)\n");
        return 1;
    }
    int braincrash = 0;
    const char* fname = argv[1];
    if (fname[0] == ':')
    {
        braincrash = 1;
        fname++;
    }

    f = fopen(fname, "rb");
    if (!f)
    {
        fprintf(stderr, "Cannot open file: %s", fname);
        return 2;
    }

    l = filelength(fileno(f));
    x = p = malloc(l);
    memset(p, 0, l);
    da = malloc(0x20000);
    memset(da, 0, 0x20000);
    a = da + 0x10000;

    while ((c = fgetc(f)) != EOF)
    {
        switch (c)
        {
            case '>':
            case '<':
            case '+':
            case '-':
            case '.':
            case ',':
                *(x++) = c;
                break;
            case '|':
            case '&':
            case '~':
            case '^':
                if (braincrash)
                    *(x++) = c;
                break;
            case '[':
                i++;
                *(x++) = c;
                break;
            case ']':
                i--;
                *(x++) = c;
                break;
            case '!':
            case '%':
                for (; (c = fgetc(f)) != '\n' && c != EOF;)
                    ;
                break;
        }
    }

    fclose(f);
    if (i)
    {
        fprintf(stderr, "COMPILE ERROR\n");
    }
    else
    {
        if (braincrash)
            strcpy((char*)a, "Hello, world!");
        execute(p, braincrash);
        printf("\n");
    }
    free(p);
    free(da);
    return 0;
}
