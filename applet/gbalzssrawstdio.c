#include <stdio.h>
#include "../lib/libmshlsplash.h"

int gbalzssrawstdio(const int argc, const char** argv)
{
    char mode, vram;
    if (argc < 2)
        goto argerror;
    mode = argv[1][0], vram = argv[1][1];
    if (!mode)
        goto argerror;
    if (mode == '-')
        mode = argv[1][1], vram = argv[1][2];
    if (mode != 'e' && mode != 'c' && mode != 'd')
        goto argerror;
    if (isatty(fileno(stdin)) || isatty(fileno(stdout)))
        goto argerror;

    mode == 'd' ? gbalzssDecodeFile(stdin, stdout) : gbalzssEncodeFile(stdin, stdout, vram ? true : false);
    return 0;

argerror:
    fprintf(stderr, "gbalzssrawstdio e/d < in > out\n"
                    "You can also use -e,-c,-d.\n"
                    "in compression, in must be a file, not stream\n");
    return -1;
}
