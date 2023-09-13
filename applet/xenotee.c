#include "../xenobox.h"
int xenotee(const int argc, const char** argv)
{
    if (argc < 2 || isatty(fileno(stdin)))
    {
        fprintf(stderr, "program-to-pipe|xenotee [:]file\n"
                        "copy stdin to file\n"
                        "use :file to append\n");
        return -1;
    }
    const char *mode = "wb", *file = argv[1];
    int c;
    if (file[0] == ':')
        mode = "ab", file = argv[1] + 1;
    FILE* f = fopen(file, mode);
    if (!f)
    {
        fprintf(stderr, "failed to open file\n");
        return 1;
    }
    for (; ~(c = getchar());)
        putchar(c), fputc(c, f);
    fclose(f);
    return 0;
}