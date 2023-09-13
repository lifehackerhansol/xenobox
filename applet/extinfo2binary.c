// never execute on big endian machine!

#include "../xenobox.h"

static int txt2bin(const char* src, unsigned char* dst)
{ // from m3patch
    int i = 0;
    for (; i < 4; i++)
    {
        unsigned char src0 = src[2 * i];
        if (!src0 || src0 == ' ' || src0 == '\t' || src0 == '\r' || src0 == '\n' || src0 == '#' || src0 == ';' ||
            src0 == '\'' || src0 == '"')
            break;
        if (0x60 < src0 && src0 < 0x7b)
            src0 -= 0x20;
        if (!(isdigit(src0) || (0x40 < src0 && src0 < 0x47)))
        {
            fprintf(stderr, "Invalid character %c\n", src0);
            exit(-1);
        }
        src0 = isdigit(src0) ? (src0 - '0') : (src0 - 55);

        unsigned char src1 = src[2 * i + 1];
        if (0x60 < src1 && src1 < 0x7b)
            src1 -= 0x20;
        if (!(isdigit(src1) || (0x40 < src1 && src1 < 0x47)))
        {
            fprintf(stderr, "Invalid character %c\n", src1);
            exit(-1);
        }
        src1 = isdigit(src1) ? (src1 - '0') : (src1 - 55);
        dst[i] = (src0 << 4) | src1;
        // fprintf(stderr,"%02X",dst[i]);
    }
    return i;
    // fprintf(stderr,"\n");
}

typedef struct
{
    unsigned int gamecode;
    unsigned int crc32;
} extinfoindex;

static unsigned char head[512];
static int stage2(unsigned int* p, FILE* nds)
{
    unsigned int arm9offset = read32(head + 0x20);
    unsigned int arm9address = read32(head + 0x28);
    unsigned int arm9end = arm9address + read32(head + 0x2c);
    unsigned int arm9reloc = arm9offset - arm9address;
    unsigned int arm7offset = read32(head + 0x30);
    unsigned int arm7address = read32(head + 0x38);
    unsigned int arm7end = arm7address + read32(head + 0x3c);
    unsigned int arm7reloc = arm7offset - arm7address;
    if (nds)
    {
        fprintf(stderr, "ARM9: %08x %08x-%08x reloc=%08x\n", arm9offset, arm9address, arm9end, arm9reloc);
        fprintf(stderr, "ARM7: %08x %08x-%08x reloc=%08x\n", arm7offset, arm7address, arm7end, arm7reloc);
    }

    unsigned int count = *p++;
    unsigned int specialpatch = -1;
    unsigned int i, j;

    unsigned int* q;
    unsigned int address;
    unsigned int length;
    q = p;
    for (i = 0; i < count; q += length, i++)
    { // find 0xd0000000
        address = *q++;
        length = *q++;
        if (length & 3)
        {
            fprintf(stderr, "length isn't 4*n\n");
            return 1;
        }
        length /= 4;
        if (address == 0xd0000000)
        {
            specialpatch = i;
        } // break;}
    }

    q = p;
    for (i = 0; i < count; q += length, i++)
    {
        address = *q++;
        length = *q++;
        fprintf(stderr, "Patch %u: %dbytes %08X", i, length, address);
        // if(length&3){printf("length isn't 4*n\n");return 1;}
        length /= 4;

        if (specialpatch != -1 && i <= specialpatch)
        {
            fprintf(stderr, "\n");
            if (i == specialpatch)
                fprintf(stderr, "[above is DSTT specific patch]\n");
            continue;
        }

        unsigned int address_reloc = address;
        if (arm9address <= address && address < arm9end)
            address_reloc += arm9reloc;
        else if (arm7address <= address && address < arm7end)
            address_reloc += arm7reloc;

        if (nds)
        {
            if (address_reloc != address)
            {
                fprintf(stderr, "->%08X\n", address_reloc);
            }
            else
            {
                fprintf(stderr, " cannot patch, DSi address?\n");
            }
        }
        else
            fprintf(stderr, "\n");

        if (nds)
            fseek(nds, address_reloc, SEEK_SET);
        // info writer
        for (j = 0; j < length; j++)
        {
            if (address_reloc != address)
            {
                fprintf(stderr, "[+] %08x %08x\n", address_reloc + j * 4, q[j]);
                // if(nds)fwrite(q+j,1,4,nds);
            }
            else
            {
                fprintf(stderr, "[*] %08x %08x\n", address + j * 4, q[j]);
#if 0
				//if(nds)fseek(nds,0x4590+j*4,SEEK_SET);
				//if(nds)fwrite(q+j,1,4,nds);
#endif
            }
        }

        // cheat code writer (disabled)
        if (length >= 4)
        {
            // write32(head,0xe0000000|address);fwrite(head,1,4,stdout);
            // write32(head,length*4);fwrite(head,1,4,stdout);

            // printf("%08X %08X\n",0xe0000000|address,(length&1)?((length-1)*4):(length*4));
            for (j = 0; j + 1 < length; j += 2)
            {
                // printf("%08X %08X\n",q[j],q[j+1]);
            }
            if (length & 1)
            {
                // printf("%08X %08X\n",address+j*4,q[j]);
            }
        }
        else
        {
            for (j = 0; j < length; j++)
            {
                // printf("%08X %08X\n",address+j*4,q[j]);
            }
        }
    }
    return 0;
}

static int stage1(FILE* extinfo, FILE* nds, unsigned int gamecode, unsigned int CRC32)
{ // parse extinfo header
    unsigned int count;
    fseek(extinfo, 12, SEEK_SET);
    fread(&count, 1, 4, extinfo);

    unsigned int i = 0;
    extinfoindex ext;
    fseek(extinfo, 32, SEEK_SET);
    fprintf(stderr, "[%c%c%c%c:%08x] ", gamecode & 0xff, (gamecode >> 8) & 0xff, (gamecode >> 16) & 0xff,
            (gamecode >> 24), CRC32);
    for (; i < count; i++)
    {
        fread(&ext, 1, sizeof(ext), extinfo);
        ext.gamecode ^= 0xffffffff;
        ext.crc32 ^= 0xffffffff;
        if (ext.gamecode == gamecode && (ext.crc32 == CRC32 || !ext.crc32 || !CRC32))
        { // buggy?
            fseek(extinfo, 0x20 + 8 * count + 8 * i, SEEK_SET);
            unsigned int offset, size;
            fread(&offset, 1, 4, extinfo);
            fread(&size, 1, 4, extinfo);
            offset ^= 0xffffffff, size ^= 0xffffffff;
            if (size & 3)
            {
                fprintf(stderr, "size isn't 4*n\n");
                return 1;
            }
            fprintf(stderr, "%08X %dbytes\n", offset, size);
            unsigned int* p = (unsigned int*)malloc(size);
            fseek(extinfo, offset, SEEK_SET);
            fread(p, 1, size, extinfo);
            unsigned int j = 0;
            for (; j < size / 4; j++)
                p[j] ^= 0xffffffff;
            // FILE *x=fopen("ext.bin","wb");fwrite(p,1,size,x);fclose(x);
            stage2(p, nds);
            free(p);
            break;
        }
    }
    if (i == count)
        fprintf(stderr, "not found.\n");
    return 0;
}

int extinfo2binary(const int argc, const char** argv)
{
    FILE *extinfo = NULL, *nds = NULL;
    if (argc < 3)
    {
        fprintf(stderr, "extinfo2cheat extinfo.dat [game.nds|GAME:aabbccdd]...\n");
        return 1;
    }
    if (!(extinfo = fopen(argv[1], "rb")))
    {
        fprintf(stderr, "cannot open extinfo\n");
        return 2;
    }
    fread(head, 1, 8, extinfo);
    if (memcmp(head, "ExtInfo", 8))
    {
        fprintf(stderr, "not extinfo\n");
        fclose(extinfo);
        return 3;
    }
    int i = 2;
    for (; i < argc; i++)
    {
        if (argv[i][4] != ':')
        {
            fprintf(stderr, "%s: ", argv[i]);
            if (!(nds = fopen(argv[i], "r+b")))
            {
                fprintf(stderr, "cannot open nds\n");
                continue;
            }
            fread(head, 1, 512, nds);
            memset(head + 0x160, 0, 0xa0); /////
            stage1(extinfo, nds, read32(head + 12), crc32(0, head, 512) ^ 0xffffffff);
            fclose(nds);
        }
        else
        {
            if (strlen(argv[i]) != 13)
            {
                fprintf(stderr, "argument format error\n");
                continue;
            }
            memset(head, 0, 512);
            unsigned char crc_bin[4], c;
            txt2bin(argv[i] + 5, crc_bin);
            c = crc_bin[0], crc_bin[0] = crc_bin[3], crc_bin[3] = c;
            c = crc_bin[1], crc_bin[1] = crc_bin[2], crc_bin[2] = c;
            stage1(extinfo, NULL, read32(argv[i]), read32(crc_bin));
        }
    }
    fclose(extinfo);
    return 0;
}
