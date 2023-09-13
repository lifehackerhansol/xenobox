#include "../xenobox.h"

static char label[1024];
static void formatlabel()
{
    int i = 0;
    if ('0' <= label[0] && label[0] <= '9')
        label[0] = '_';
    for (; i < strlen(label); i++)
        if (label[i] == '.')
            label[i] = '_';
}

static void bin2c(FILE* in)
{
    int c, i = 0;
    printf("unsigned char %s[] = {\n", label);
    for (; ~(c = fgetc(in));)
    {
        if (i == 0)
            putchar('\t');
        printf("0x%02x,", c);
        if (i == 15)
            putchar('\n');
        i = (i + 1) % 16;
    }
    if (i)
        putchar('\n');
    printf("};\nunsigned int %s_size = %u;\n", label, (u32)ftell(in));

    if (!isatty(fileno(stderr)))
    {
        fprintf(stderr,
                "extern unsigned char %s[];\n"
                "extern unsigned int %s_size;\n",
                label, label);
    }
}

int bin2cstdio(const int argc, const char** argv)
{
    FILE* f;

    if (argc < 2)
    {
        fprintf(stderr, "bin2cstdio in... >out [2>head]\n");
        fprintf(stderr, "bin2cstdio label <in >out [2>head]\n");
        return -1;
    }

    if (!isatty(fileno(stdin)))
    {
        strcpy(label, argv[1]);
        formatlabel();
        bin2c(stdin);
    }
    else
    {
        int i = 1;
        for (; i < argc; i++)
        {
            // fprintf(stderr,"%s: ",argv[i]);
            f = fopen(argv[i], "rb+");
            if (!f)
            {
                // fprintf(stderr,"Cannot open\n");
                continue;
            }
            strcpy(label, argv[i]);
            formatlabel();
            bin2c(f);
            fclose(f);
        }
    }
    return 0;
}
