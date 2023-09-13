#include "../xenobox.h"

int modifybanner(const int argc, const char** argv)
{
    unsigned char head[2112];
    unsigned short banner[128];

    if (argc < 3)
    {
        printf("modifybanner nds banner(UTF8)\nex) modifybanner woodex4.nds \"Wood EX4;for R4iLS/EX4DS\"");
        return 1;
    }
    memset(banner, 0, 256);
    mbstoucs2(banner, (u8*)argv[2]);
    for (unsigned int i = 0; i < 128; i++)
        if (banner[i] == ';')
            banner[i] = '\n';
    FILE* f = fopen(argv[1], "r+b");
    if (!f)
    {
        printf("cannot open nds\n");
        return 2;
    }
    fread(head, 1, 0x6c, f);
    unsigned int offset = read32(head + 0x68);
    if (!offset)
    {
        fclose(f);
        printf("nds doesn't have banner\n");
        return 3;
    }
    fseek(f, offset, SEEK_SET);
    fread(head, 1, 2112, f);
    for (unsigned int i = 0; i < 6; i++)
        memcpy(head + 0x240 + 0x100 * i, banner, 256);
    write16(head + 2, crc16(0xffff, head + 32, 2080));
    fseek(f, offset, SEEK_SET);
    fwrite(head, 1, 2112, f);
    fclose(f);
    return 0;
}
