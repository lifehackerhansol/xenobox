#include "../xenobox.h"

int guidpatch(const int argc, const char** argv)
{
    FILE* f;
    unsigned char *p, *q;
    int size;
    int arm9body, arm9mem, arm9size;
    int arm7body, arm7mem, arm7size;
    unsigned int xcrc = 0, crcoffset = -1, flag = 0;

    fprintf(stderr, "MoonShell GUID Patch in C\n\n");
    if (!(f = fopen(argv[1], "rb+")))
    {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
        return 1;
    }
    size = filelength(fileno(f));
    if (!(p = malloc(size)))
    {
        fprintf(stderr, "Cannot allocate %d bytes\n", size);
        return 1;
    }
    fread(p, 1, size, f);
    rewind(f);

    ///
    fprintf(stderr, "arm9body=0x%08x\n", arm9body = read32(p + 0x20));
    fprintf(stderr, "arm9memstart=0x%08x\n", arm9mem = read32(p + 0x28));
    fprintf(stderr, "arm9memend=0x%08x\n", arm9mem + (arm9size = read32(p + 0x2c)));

    for (q = p + arm9body; q < p + arm9body + arm9size; q += 4)
    {
        if (read32(q) == 0x44495547)
        { // skip GUID
            if (read32(q + 4) == 0x47554944)
                crcoffset = q + 8 - p;
            q += 20;
        }
        else if (read32(q) == 0xbf8da5ed)
            q += 32 * 1024 - 4; // skip DLDI
        else
            xcrc = (xcrc << 1 | xcrc >> 31) ^ read32(q);
    }
    fprintf(stderr, "xcrc=0x%08x\n", xcrc);
    fprintf(stderr, "crcoffset=0x%08x\n", crcoffset);
    if (~crcoffset)
    {
        write32(p + crcoffset, xcrc);
        write32(p + crcoffset + 4, arm9mem);
        write32(p + crcoffset + 8, arm9mem + arm9size);
        flag = 1;
        fprintf(stderr, "ARM9: Patched successfully\n");
    }
    else
    {
        fprintf(stderr, "ARM9: GUIDDIUG not found\n");
    }

    ///
    xcrc = 0, crcoffset = -1;
    fprintf(stderr, "arm7body=0x%08x\n", arm7body = read32(p + 0x30));
    fprintf(stderr, "arm7memstart=0x%08x\n", arm7mem = read32(p + 0x38));
    fprintf(stderr, "arm7memend=0x%08x\n", arm7mem + (arm7size = read32(p + 0x3c)));

    for (q = p + arm7body; q < p + arm7body + arm7size; q += 4)
    {
        if (read32(q) == 0x44495547)
        { // skip GUID
            if (read32(q + 4) == 0x47554944)
                crcoffset = q + 8 - p;
            q += 20;
        }
        else if (read32(q) == 0xbf8da5ed)
            q += 32 * 1024 - 4; // skip DLDI
        else
            xcrc = (xcrc << 1 | xcrc >> 31) ^ read32(q);
    }
    fprintf(stderr, "xcrc=0x%08x\n", xcrc);
    fprintf(stderr, "crcoffset=0x%08x\n", crcoffset);
    if (~crcoffset)
    {
        write32(p + crcoffset, xcrc);
        write32(p + crcoffset + 4, arm7mem);
        write32(p + crcoffset + 8, arm7mem + arm7size);
        flag = 1;
        fprintf(stderr, "ARM7: Patched successfully\n");
    }
    else
    {
        fprintf(stderr, "ARM7: GUIDDIUG not found\n");
    }

    if (flag)
        fwrite(p, 1, size, f);
    free(p);
    fclose(f);
    return 0;
}
