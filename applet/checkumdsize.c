#include "../xenobox.h"

int checkumdsize(const int argc, const char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "checkumdsize DISC.iso...\n"
                        "If you see Over (...) and you think it is correct\n"
                        "(by checking with UMDGen), try:\n"
                        "xenobox ftruncate DISC.iso sizeFromSectors\n");
        return -1;
    }
    int i = 1;
    for (; i < argc; i++)
    {
        printf("%s:\n", argv[i]);
        FILE* f = fopen(argv[i], "rb");
        if (!f)
        {
            printf("fopen failed\n");
            continue;
        }
        fseek(f, 0x8050, SEEK_SET);
        fread(buf, 1, 16, f);
        unsigned int sectors = read32(buf);
        if (buf[0] != buf[7] || buf[1] != buf[6] || buf[2] != buf[5] || buf[3] != buf[4])
        {
            fclose(f);
            printf("not valid ISO\n");
            continue;
        }
        unsigned int sizeFromSectors = sectors * 2048, sizeFromStat = filelength(fileno(f));
        fclose(f);
        printf("filesize=%u, sizeFromSectors=%u ", sizeFromStat, sizeFromSectors);
        if (sizeFromStat == sizeFromSectors)
            printf("OK\n");
        if (sizeFromStat < sizeFromSectors)
            printf("Bad (-%u)\n", sizeFromSectors - sizeFromStat);
        if (sizeFromStat > sizeFromSectors)
            printf("Over (%u)\n", sizeFromStat - sizeFromSectors);
    }
    return 0;
}
