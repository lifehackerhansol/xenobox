/*
    dldipatch aka dlditool public domain
    under Creative Commons CC0

    According to ndsdis2 -NH9 0x00 dldi_startup_patch.o (from NDS_loader build result):
    :00000040 E3A00001 mov  r0,#0x1 ;r0=1(0x1)
    :00000044 E12FFF1E bx r14 (Jump to addr_00000000?)
    So the corresponding memory value is "\x01\x00\xa0\xe3\x1e\xff\x2f\xe1" (8 bytes).

*/

#include "../xenobox.h"

// ARM7 not officially supported
#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
#include <nds.h>
#include "_console.h"
#define printf _consolePrintf
#else
#define printf PRINTF
#include <stdarg.h>
int PRINTF(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vfprintf(stderr, format, args);
    va_end(args);
    return ret;
}
#endif

#define magicString    0x00
#define dldiVersion    0x0c
#define driverSize     0x0d
#define fixSections    0x0e
#define allocatedSpace 0x0f
#define friendlyName   0x10

#define dataStart 0x40
#define dataEnd   0x44
#define glueStart 0x48
#define glueEnd   0x4c
#define gotStart  0x50
#define gotEnd    0x54
#define bssStart  0x58
#define bssEnd    0x5c

#define ioType       0x60
#define dldiFeatures 0x64
#define dldiStartup  0x68
#define isInserted   0x6c
#define readSectors  0x70
#define writeSectors 0x74
#define clearStatus  0x78
#define shutdown     0x7c
#define dldiData     0x80

#define fixAll  0x01
#define fixGlue 0x02
#define fixGot  0x04
#define fixBss  0x08

const byte* dldimagic = (byte*)"\xed\xa5\x8d\xbf Chishm";

#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
int tunedldi(const char* name, const char* id, int* size, byte** p, int checkstart)
{
    FILE* f;
    byte* x;
    struct stat st;
    if (!(f = fopen(name, "rb")))
        return 1;
    fstat(fileno(f), &st);
    if (st.st_size < 0x80 || !(x = malloc(st.st_size)))
    {
        fclose(f);
        return 2;
    }
    fread(x, 1, st.st_size, f);
    fclose(f);
    if (memcmp(x + ioType, id, 4))
    {
        free(x);
        return 3;
    }
    if (checkstart &&
        !memcmp(x + (*(u32*)(x + dldiStartup) - *(u32*)(x + dataStart)), "\x01\x00\xa0\xe3\x1e\xff\x2f\xe1", 8))
    {
        free(x);
        return 4;
    }
    *p = x;
    *size = *((u32*)(x + bssStart)) - *((u32*)(x + dataStart));
    return 0;
}
#endif

#define torelative(n) (read32(pA + n) - pAdata)

int dldi(byte* nds, const int ndslen, const int ignoresize, const byte* pD, const int dldilen)
{
#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
    byte* pD = NULL;
    int dldilen;
    const byte* DLDIDATA = io_dldi_data;
    // const byte *DLDIDATA=((u32*)(&_io_dldi))-24;
#endif

    byte *pA = NULL, id[5], space;
    u32 reloc, pAdata, pDdata, pDbssEnd, fix;
    int i, ittr;

    for (i = 0; i < ndslen - 0x80; i += 4)
    {
        if (!memcmp(nds + i, dldimagic, 12) && (read32(nds + i + dldiVersion) & 0xe0f0e0ff) == 1)
        {
            pA = nds + i;
            PRINTF("Section 0x%08x: ", i);
            if (*((u32*)(pD + bssEnd)) - *((u32*)(pD + dataStart)) > 1 << pA[allocatedSpace])
            {
                PRINTF("Available %dB, need %dB. ", 1 << pA[allocatedSpace],
                       *((u32*)(pD + bssEnd)) - *((u32*)(pD + dataStart)));
                if (ignoresize)
                {
                    PRINTF("searching interrupted.\n");
                    break;
                }
                PRINTF("continue searching.\n");
                pA = NULL;
                continue;
            }
            PRINTF("searching done.\n");
            break;
        }
    }
    if (!pA)
    {
        PRINTF("not found valid dldi section\n");
        return 1;
    }

#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
    // Now we have to tune in the dldi...
    pD = (byte*)DLDIDATA;
    memcpy(id, pD + ioType, 4);
    id[4] = 0;
    /*
        {
            int idx=0;
            for(;idx<32*1024/4;idx++)
                if(pD[idx]!=0)dldilen=(idx+1)*4; //BackupDLDIBody() in MoonShell 2.00beta5
        }
    */
    dldilen = *((u32*)(pD + bssStart)) - *((u32*)(pD + dataStart)); // DLDITool 0.32.4

    if (!memcmp(id, "RPGS", 4) && !memcmp(pD + friendlyName, "Acekard AK2", 11))
    {
        pD = NULL;
        PRINTF("Oh dear. I'm patched with akaio DLDI.\n"
               "YSMenu will get upset and burry me.\n"
               "I have to beg a pardon\n"
               "by feeding akmenu DLDI.\n");
        if (tunedldi("/__AK2/AK2_SD.DLDI", (char*)id, &dldilen, &pD, 0))
        {
            PRINTF("Cannot load /__AK2/AK2_SD.DLDI.\n");
            goto akaiofail;
        }
        if (!memcmp(pD + friendlyName, "Acekard AK2", 11))
        {
            PRINTF("/__AK2/AK2_SD.DLDI is also akaio DLDI. What a shame!\n");
            goto akaiofail;
        }
        goto done;

    akaiofail:
        free(pD);
        PRINTF("I cannot do anything. I have to run away!\n\nNobody knows where he is now...\n");
        while (1)
            ;
    }

    if (memcmp(*(void**)(pD + dldiStartup), "\x01\x00\xa0\xe3\x1e\xff\x2f\xe1", 8)) // z=="mov r0,#1;bx lr"
        goto done;

    PRINTF("Startup is nullified. Cannot be used for patching. Trying to fall back to MoonShell2.\n");
    if (memcmp(pD + (*(u32*)(pD + dldiStartup) - *(u32*)(pD + dataStart)), "\x01\x00\xa0\xe3\x1e\xff\x2f\xe1", 8))
    {
        PRINTF("Startup is not nullified by alternative calculation. Something is strange. Halted.\n");
        while (1)
            ;
    }
    tunedldi("/MOONSHL2/DLDIBODY.BIN", (char*)id, &dldilen, &pD, 1);
    PRINTF("Tuned. Now we selected dldi file to patch with.\n");
done:
#endif
    /*
        if(*((u32*)(pD+bssEnd))-*((u32*)(pD+dataStart)) > 1<<pA[allocatedSpace])
            {PRINTF("not enough space. available %d bytes, need %d
       bytes\n",1<<pA[allocatedSpace],*((u32*)(pD+bssEnd))-*((u32*)(pD+dataStart)));return 2;}
    */
    space = pA[allocatedSpace];

    pAdata = read32(pA + dataStart);
    if (!pAdata)
        pAdata = read32(pA + dldiStartup) - dldiData;
    memcpy(id, pA + ioType, 4);
    id[4] = 0;
    PRINTF("Old ID=%s, Interface=0x%08x,\nName=%s\n", id, pAdata, pA + friendlyName);
    memcpy(id, pD + ioType, 4);
    id[4] = 0;
    PRINTF("New ID=%s, Interface=0x%08x,\nName=%s\n", id, pDdata = read32(pD + dataStart), pD + friendlyName);
    PRINTF("Relocation=0x%08x, Fix=0x%02x\n", reloc = pAdata - pDdata, fix = pD[fixSections]); // pAdata=pDdata+reloc
    PRINTF("dldiFileSize=0x%04x, dldiMemSize=0x%04x\n", dldilen, *((u32*)(pD + bssEnd)) - *((u32*)(pD + dataStart)));

    memcpy(pA, pD, dldilen);
    pA[allocatedSpace] = space;
    for (ittr = dataStart; ittr < ioType; ittr += 4)
        write32(pA + ittr, read32(pA + ittr) + reloc);
    for (ittr = dldiStartup; ittr < dldiData; ittr += 4)
        write32(pA + ittr, read32(pA + ittr) + reloc);
    pAdata = read32(pA + dataStart);
    pDbssEnd = read32(pD + bssEnd);

    if (fix & fixAll)
        for (ittr = torelative(dataStart); ittr < torelative(dataEnd); ittr += 4)
            if (pDdata <= read32(pA + ittr) && read32(pA + ittr) < pDbssEnd)
                PRINTF("All  0x%04x: 0x%08x -> 0x%08x\n", ittr, read32(pA + ittr), read32(pA + ittr) + reloc),
                    write32(pA + ittr, read32(pA + ittr) + reloc);
    if (fix & fixGlue)
        for (ittr = torelative(glueStart); ittr < torelative(glueEnd); ittr += 4)
            if (pDdata <= read32(pA + ittr) && read32(pA + ittr) < pDbssEnd)
                PRINTF("Glue 0x%04x: 0x%08x -> 0x%08x\n", ittr, read32(pA + ittr), read32(pA + ittr) + reloc),
                    write32(pA + ittr, read32(pA + ittr) + reloc);
    if (fix & fixGot)
        for (ittr = torelative(gotStart); ittr < torelative(gotEnd); ittr += 4)
            if (pDdata <= read32(pA + ittr) && read32(pA + ittr) < pDbssEnd)
                PRINTF("Got  0x%04x: 0x%08x -> 0x%08x\n", ittr, read32(pA + ittr), read32(pA + ittr) + reloc),
                    write32(pA + ittr, read32(pA + ittr) + reloc);
    if (fix & fixBss)
        memset(pA + torelative(bssStart), 0, pDbssEnd - read32(pD + bssStart));

#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
    if (pD && pD != DLDIDATA)
        free(pD);
#endif

    PRINTF("Patched successfully\n");
    return 0;
}

#undef torelative
#define torelative(n) (read32(pD + n) - pDdata)

#define TWICE(e) (e), (e)
int dldishow(const byte* pD, const int dldilen)
{
    byte id[5];
    u32 pDdata, pDbssEnd, fix;
    int ittr;

    pDdata = read32(pD + dataStart);
    pDbssEnd = read32(pD + bssEnd);
    memcpy(id, pD + ioType, 4);
    id[4] = 0;
    PRINTF("ID=%s, Fix=0x%02x, Features=0x%02x\nName=%s\n\n", id, fix = pD[fixSections], *((u32*)(pD + dldiFeatures)),
           pD + friendlyName);
    PRINTF("data = 0x%04x(%d)-0x%04x(%d)\n", TWICE(*((u32*)(pD + dataStart)) - *((u32*)(pD + dataStart))),
           TWICE(*((u32*)(pD + dataEnd)) - *((u32*)(pD + dataStart))));
    PRINTF("glue = 0x%04x(%d)-0x%04x(%d)\n", TWICE(*((u32*)(pD + glueStart)) - *((u32*)(pD + dataStart))),
           TWICE(*((u32*)(pD + glueEnd)) - *((u32*)(pD + dataStart))));
    PRINTF("got  = 0x%04x(%d)-0x%04x(%d)\n", TWICE(*((u32*)(pD + gotStart)) - *((u32*)(pD + dataStart))),
           TWICE(*((u32*)(pD + gotEnd)) - *((u32*)(pD + dataStart))));
    PRINTF("bss  = 0x%04x(%d)-0x%04x(%d)\n", TWICE(*((u32*)(pD + bssStart)) - *((u32*)(pD + dataStart))),
           TWICE(*((u32*)(pD + bssEnd)) - *((u32*)(pD + dataStart))));
    PRINTF("\n");
    PRINTF("dldiStartup  = 0x%04x(%d)\n", TWICE(*((u32*)(pD + dldiStartup)) - *((u32*)(pD + dataStart))));
    PRINTF("isInserted   = 0x%04x(%d)\n", TWICE(*((u32*)(pD + isInserted)) - *((u32*)(pD + dataStart))));
    PRINTF("readSectors  = 0x%04x(%d)\n", TWICE(*((u32*)(pD + readSectors)) - *((u32*)(pD + dataStart))));
    PRINTF("writeSectors = 0x%04x(%d)\n", TWICE(*((u32*)(pD + writeSectors)) - *((u32*)(pD + dataStart))));
    PRINTF("clearStatus  = 0x%04x(%d)\n", TWICE(*((u32*)(pD + clearStatus)) - *((u32*)(pD + dataStart))));
    PRINTF("shutdown     = 0x%04x(%d)\n", TWICE(*((u32*)(pD + shutdown)) - *((u32*)(pD + dataStart))));

    // for(ittr=dataStart;ittr<ioType;ittr+=4)write32(pA+ittr,read32(pA+ittr)+reloc);
    // for(ittr=dldiStartup;ittr<dldiData;ittr+=4)write32(pA+ittr,read32(pA+ittr)+reloc);
    // pAdata=read32(pA+dataStart);pDbssEnd=read32(pD+bssEnd);

    if (fix & fixAll)
        for (ittr = torelative(dataStart); ittr < torelative(dataEnd); ittr += 4)
            if (pDdata <= read32(pD + ittr) && read32(pD + ittr) < pDbssEnd)
                PRINTF("All  0x%04x: 0x%08x\n", ittr, torelative(ittr));
    if (fix & fixGlue)
        for (ittr = torelative(glueStart); ittr < torelative(glueEnd); ittr += 4)
            if (pDdata <= read32(pD + ittr) && read32(pD + ittr) < pDbssEnd)
                PRINTF("Glue 0x%04x: 0x%08x\n", ittr, torelative(ittr));
    if (fix & fixGot)
        for (ittr = torelative(gotStart); ittr < torelative(gotEnd); ittr += 4)
            if (pDdata <= read32(pD + ittr) && read32(pD + ittr) < pDbssEnd)
                PRINTF("Got  0x%04x: 0x%08x\n", ittr, torelative(ittr));

    if (!isatty(fileno(stdout)))
    {
        fwrite(pD, 1, *((u32*)(pD + bssStart)) - *((u32*)(pD + dataStart)), stdout);
    }
    return 0;
}

#if defined(FEOS) || !(defined(ARM9) || defined(ARM7))
int dldipatch(const int argc, const char** argv)
{ // for PC main()
    int i, dldisize;
    FILE *f, *fdldi;
    struct stat st, stdldi;
    byte *p, *pdldi, *pd = NULL;

    if (argc < 2)
    {
        PRINTF("dldipatch aka dlditool public domain v8\n");
        PRINTF("dldipatch dldi/homebrew [homebrew...]\n");
        PRINTF("dldipatch dldi/homebrew >dldicaptor.dldi\n");
        PRINTF("From v8 homebrew can be used as template.\n");
        return 1;
    }
    if (!(fdldi = fopen(argv[1], "rb")))
    {
        PRINTF("cannot open %s\n", argv[1]);
        return 2;
    }
    fstat(fileno(fdldi), &stdldi);
    dldisize = stdldi.st_size;
    if (!(pdldi = malloc(dldisize)))
    {
        fclose(fdldi);
        PRINTF("cannot allocate %d bytes for dldi\n", (int)dldisize);
        return 3;
    }
    fread(pdldi, 1, dldisize, fdldi);
    fclose(fdldi);

    for (i = 0; i < dldisize - 0x80; i += 4)
    {
        if (!memcmp(pdldi + i, dldimagic, 12) && (read32(pdldi + i + dldiVersion) & 0xe0f0e0ff) == 1)
        {
            pd = pdldi + i;
            dldisize -= i;
            break;
        }
    }
    if (!pd)
    {
        PRINTF("the dldi file is invalid\n");
        return 4;
    }
    u32 dldilen = *((u32*)(pd + bssStart)) - *((u32*)(pd + dataStart)); // DLDITool 0.32.4
    if (argc == 2)
        dldishow(pd, dldilen);

    for (i = 2; i < argc; i++)
    {
        int ignoresize = 0;
        const char* name = argv[i];
        if (*name == ':')
            ignoresize = 1, name++;
        PRINTF("Patching %s...\n", name);
        if (!(f = fopen(name, "rb+")))
        {
            PRINTF("cannot open %s\n", name);
            continue;
        }
        fstat(fileno(f), &st);
        if (!(p = malloc(st.st_size)))
        {
            PRINTF("cannot allocate %d bytes for %s\n", (int)st.st_size, name);
            continue;
        }
        fread(p, 1, st.st_size, f);
        rewind(f);
        if (!dldi(p, st.st_size, ignoresize, pd, dldilen))
            fwrite(p, 1, st.st_size, f);
        fclose(f);
        free(p);
    }
    free(pdldi);
    return 0;
}
#endif
