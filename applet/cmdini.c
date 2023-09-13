#include "../xenobox.h"
#include "../lib/minIni.h"

int cmdini(const int argc, const char** argv)
{
    int command = 0, i, j;
    if (argc > 1)
        command = argv[1][0];
    if ('A' <= command && command <= 'Z')
        command += 0x20;
    if (!((command == 'p' && argc == 6) || (command == 'g' && argc == 5) || (command == 'k' && argc == 4) ||
          (command == 's' && argc == 3)))
    {
        fprintf(stderr, "Put:      cmdini p file.ini Section Key Value\n"
                        "Get:      cmdini g file.ini Section Key\n"
                        "Keys:     cmdini k file.ini Section\n"
                        "Sections: cmdini s file.ini\n"
                //"DefVal is only required in Get mode\n"
        );
        return -1;
    }
    if (command == 'p')
    {
        ini_puts(argv[3], argv[4], argv[5], argv[2]);
    }
    if (command == 'g')
    {
        ini_gets(argv[3], argv[4], "", cbuf, BUFLEN, argv[2]);
        for (j = 0; cbuf[j]; j++)
            putchar(cbuf[j]);
    }
    if (command == 'k')
    {
        for (i = 0; ini_getkey(argv[3], i, cbuf, BUFLEN, argv[2]); putchar('\n'), i++)
            for (j = 0; cbuf[j]; j++)
                putchar(cbuf[j]);
    }
    if (command == 's')
    {
        for (i = 0; ini_getsection(i, cbuf, BUFLEN, argv[2]); putchar('\n'), i++)
            for (j = 0; cbuf[j]; j++)
                putchar(cbuf[j]);
    }
    return 0;
}
