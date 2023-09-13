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

static void bin2s(FILE* in)
{
    int c, i = 0;
    printf(
        //"	.section .data\n"
        "	.align 4\n"
        "	.global %s\n"
        "	.global %s_end\n"
        "	.global %s_size\n"
        "%s:\n",
        label, label, label, label);
    for (; ~(c = fgetc(in));)
    {
        if (i == 0)
            printf("\t.byte ");
        else
            putchar(',');
        printf("%3d", c);
        if (i == 15)
            putchar('\n');
        i = (i + 1) % 16;
    }
    if (i)
        putchar('\n');
    printf("%s_end:\n"
           "	.align\n"
           "%s_size: .int %u\n",
           label, label, (u32)ftell(in));

    if (!isatty(fileno(stderr)))
    {
        fprintf(stderr,
                "extern unsigned char %s[];\n"
                "extern unsigned char %s_end[];\n"
                "extern unsigned int %s_size;\n",
                label, label, label);
    }
}

int bin2sstdio(const int argc, const char** argv)
{
    FILE* f;

    if (argc < 2)
    {
        fprintf(stderr, "bin2sstdio in... >out [2>head]\n");
        fprintf(stderr, "bin2sstdio label <in >out [2>head]\n");
        return -1;
    }

    if (!isatty(fileno(stdin)))
    {
        strcpy(label, argv[1]);
        formatlabel();
        bin2s(stdin);
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
            bin2s(f);
            fclose(f);
        }
    }
    return 0;
}
