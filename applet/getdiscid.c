#include "../xenobox.h"

// due to malloc thing, using on portable devices will have difficulty.

unsigned char __compbuf[COMPBUFLEN], __decompbuf[DECOMPBUFLEN];

#define bin2int(var, addr) write32(var, *(u32*)addr)

// iso_tool
#include "getdiscid/file.c"
#include "getdiscid/iso.c"
#include "getdiscid/ciso.c"

#include "getdiscid/dax.c"
#include "getdiscid/jiso.c"

s32 MAX_SECTOR_NUM;
char* WORK; /*[MAX_SECTOR_NUM][SECTOR_SIZE]; __attribute__((aligned(0x40))); */
static char* WORK_PTR;

// frontend
static char __disc_id[256];
static const char* getebootidarg[] = {
    "getebootid",
    "",
    "",
};

int getdiscid(const int argc, const char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "getdiscid (iso_tool port)\n"
                        "getdiscid EBOOT.PBP/DISC.iso/DISC.cso/DISC.jso/DISC.dax [T|P] [>PARAM.SFO]\n"
                        "use T to get TITLE instead of DISC_ID\n"
                        "use P to dump PARAM.SFO to stdout (you need to redirect)\n"
                        "Please note DAX with NC isn't supported (will emit iso_get_file_info error).\n");
        return -1;
    }
    FILE* f = fopen(argv[1], "rb");
    if (!f)
    {
        fprintf(stderr, "fopen failed\n");
        return 2;
    }
    fread(__disc_id, 1, 8, f);
    fclose(f);

    if (!memcmp(__disc_id, "\0PBP", 4))
    {
        int _argc = 2;
        getebootidarg[1] = argv[1];
        if (argc > 2)
        {
            _argc = 3;
            getebootidarg[2] = argv[2];
        }
        return getebootid(_argc, getebootidarg);
    }

    int type = TYPE_ISO;
    if (!memcmp(__disc_id, "DAX\0", 4))
    {
        type = TYPE_DAX;
        // fprintf(stderr,"DAX not supported\n");return 2;
    }
    if (!memcmp(__disc_id, "CISO", 4))
    {
        type = TYPE_CSO;
    }
    if (!memcmp(__disc_id, "JISO", 4))
    {
        type = TYPE_JSO;
    }

    // running iso_tool backend
    MAX_SECTOR_NUM = 512 * 18 /*MB*/;
    do
    {
        WORK_PTR = malloc(MAX_SECTOR_NUM * SECTOR_SIZE + 0x3F);
        if (WORK_PTR == NULL)
        {
            MAX_SECTOR_NUM -= 512;
            if (MAX_SECTOR_NUM < 0)
            {
                fprintf(stderr, "malloc (for disc sectors) failed\n");
                abort();
            }
        }
        else
            break;
    } while (MAX_SECTOR_NUM > 0);
    WORK = (char*)((size_t)WORK_PTR & ~0x3F);

    iso_file_stat s;
    if (iso_get_file_info(&s, argv[1], type, "PSP_GAME/PARAM.SFO"))
    {
        free(WORK_PTR);
        fprintf(stderr, "iso_get_file_info failed\n");
        return 3;
    }
    char* p = malloc(s.size);
    if (!p)
    {
        free(WORK_PTR);
        fprintf(stderr, "malloc (for PARAM.SFO) failed\n");
        return 4;
    }
    if (iso_read(p, s.size, argv[1], type, "PSP_GAME/PARAM.SFO") < s.size)
    {
        free(WORK_PTR);
        fprintf(stderr, "iso_read failed\n");
        return 5;
    }
    free(WORK_PTR);

    // read PARAM.SFO
    if (memcmp(p, "\0PSF", 4) || read32(p + 4) != 0x00000101)
    {
#ifdef PSP
        sceKernelFreePartitionMemory(uid);
#else
        free(p);
#endif
        return 1;
    }

#ifndef PSP
    if (argc > 2 && (argv[2][0] == 'p' || argv[2][0] == 'P') && !isatty(fileno(stdout)))
    {
        fwrite(p, 1, s.size, stdout);
        free(p);
        return 0;
    }
#endif

    int label_offset = read32(p + 8);
    int data_offset = read32(p + 12);
    int nlabel = read32(p + 16);
    int i = 0;
    for (; i < nlabel; i++)
    {
        if (!strcmp(p + label_offset + read16(p + 20 + 16 * i), argc > 2 ? "TITLE" : "DISC_ID"))
        { // seems to be 16bytes long
            int datasize = read32(p + 20 + 16 * i + 8);
            // if(datasize>19)datasize=19;
            memcpy(__disc_id, p + data_offset + read32(p + 20 + 16 * i + 12), datasize);
            break;
        }
    }
#ifdef PSP
    sceKernelFreePartitionMemory(uid);
#else
    free(p);
#endif
    if (!isatty(fileno(stdout)))
        printf("%s", __disc_id); // can use this value in shell
    else
        puts(__disc_id);

    return 0;
}
