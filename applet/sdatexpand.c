#include "../xenobox.h"
#include "sdat.h"

#define TMPFILE "__sdatexpand_sseqinfo.tmp"

// sdatexpand - XenoBox sdat parser
// expand mode cannot be ported to portable devices (list/sps modes are ok).
//
// ndsEnumFiles() is derived from nitrofs expander by loveemu under MIT license
// http://code.google.com/p/loveemu/source/browse/nitroscrap/src/axnds/nitrofs.cpp

#define MAX_SDAT 1024

static u32 sdat_count;
static u32 sdat_iter = 0;
static u32 sdat_offset[MAX_SDAT];
static u32 sdat_size[MAX_SDAT];
static u32 sdat_head_size[MAX_SDAT];

static u8* sdat;

// getfilesize
static SDATFAT* fat;
static int getfilesize(int fileid, int* offset, int* size)
{
    if (fileid >= fat->nCount)
        return 1;
    *offset = fat->Rec[fileid].nOffset;
    *size = fat->Rec[fileid].nSize;
    return 0;
}

// getfilename
static int mode;
static char name[256];
static char* ext[8] = { "sseq", "sarc", "sbnk", "swar", "player", "group", "player2", "strm" };
static SDATSYMB* symb;
static char* getfilename(int type, int index)
{
    if (symb)
    {
        SDATSYMBREC* symbrec = (SDATSYMBREC*)((u8*)symb + symb->nRecOffset[type]);
        if (sdat_count > 1)
        {
            if (mode < 2)
                sprintf(name, "%s.%d.%s", (char*)((u8*)symb + symbrec->nEntryOffset[index]), sdat_iter + 1, ext[type]);
            else
                sprintf(name, "%s.%d", (char*)((u8*)symb + symbrec->nEntryOffset[index]), sdat_iter + 1);
        }
        else
        {
            if (mode < 2)
                sprintf(name, "%s.%s", (char*)((u8*)symb + symbrec->nEntryOffset[index]), ext[type]);
            else
                sprintf(name, "%s", (char*)((u8*)symb + symbrec->nEntryOffset[index]));
        }
        return name;
    }
    else
    {
        if (sdat_count > 1)
        {
            if (mode < 2)
                sprintf(name, "%04d.%d.%s", index, sdat_iter + 1, ext[type]);
            else
                sprintf(name, "%04d.%d", index, sdat_iter + 1);
        }
        else
        {
            if (mode < 2)
                sprintf(name, "%04d.%s", index, ext[type]);
            else
                sprintf(name, "%04d", index);
        }
        return name;
    }
}

static int sdat_entry = 0;
static FILE* finfo;
static FILE* ftmp;
static FILE* fnds;
static int parseSDAT()
{
    SDATHEADER* header = (SDATHEADER*)sdat; // already checked in main

    fat = (SDATFAT*)((u8*)header + header->nFatOffset);
    if (memcmp(fat->type, "FAT ", 4))
    {
        fprintf(stderr, "broken fat\n");
        return 1;
    }

    SDATINFO* info = (SDATINFO*)((u8*)header + header->nInfoOffset);
    if (memcmp(info->type, "INFO", 4))
    {
        fprintf(stderr, "broken info\n");
        return 2;
    }

    symb = (SDATSYMB*)((u8*)header + header->nSymbOffset);
    if (header->file.nBlock == 3 || memcmp(symb->type, "SYMB", 4))
    {
        symb = NULL;
        fprintf(stderr, "using numeric filename\n");
    }

    SDATINFOREC
    *inforec0 = (SDATINFOREC*)((u8*)info + info->nRecOffset[0]),     // sseq
        *inforec2 = (SDATINFOREC*)((u8*)info + info->nRecOffset[2]), // sbnk
        *inforec3 = (SDATINFOREC*)((u8*)info + info->nRecOffset[3]), // swar
        *inforec7 = (SDATINFOREC*)((u8*)info + info->nRecOffset[7]); // strm (can be played via vgmstream)

    int i, j;

    if (mode < 2)
    {
        // I hope fwrite will emit Segmentation Fault if size handling is buggy...
        for (i = 0; i < inforec0->nCount; i++)
        {
            int offset, size;
            if (!getfilesize(((SDATINFOSSEQ*)((u8*)info + inforec0->nEntryOffset[i]))->fileID, &offset, &size))
            {
                printf("%d %d %d %s\n", ((SDATINFOSSEQ*)((u8*)info + inforec0->nEntryOffset[i]))->fileID,
                       sdat_offset[sdat_iter] + offset, size, getfilename(0, i));
                if (mode == 1)
                {
                    FILE* f = fopen(getfilename(0, i), "wb");
                    fwrite(sdat + offset, 1, size, f);
                    fclose(f);
                }
            }
        }

        for (i = 0; i < inforec2->nCount; i++)
        {
            int offset, size;
            if (!getfilesize(((SDATINFOSBNK*)((u8*)info + inforec2->nEntryOffset[i]))->fileID, &offset, &size))
            {
                printf("%d %d %d %s\n", ((SDATINFOSBNK*)((u8*)info + inforec2->nEntryOffset[i]))->fileID,
                       sdat_offset[sdat_iter] + offset, size, getfilename(2, i));
                if (mode == 1)
                {
                    FILE* f = fopen(getfilename(2, i), "wb");
                    fwrite(sdat + offset, 1, size, f);
                    fclose(f);
                }
            }
        }

        for (i = 0; i < inforec3->nCount; i++)
        {
            int offset, size;
            if (!getfilesize(((SDATINFOSWAR*)((u8*)info + inforec3->nEntryOffset[i]))->fileID, &offset, &size))
            {
                printf("%d %d %d %s\n", ((SDATINFOSWAR*)((u8*)info + inforec3->nEntryOffset[i]))->fileID,
                       sdat_offset[sdat_iter] + offset, size, getfilename(3, i));
                if (mode == 1)
                {
                    FILE* f = fopen(getfilename(3, i), "wb");
                    fwrite(sdat + offset, 1, size, f);
                    fclose(f);
                }
            }
        }

        for (i = 0; i < inforec7->nCount; i++)
        {
            int offset, size;
            if (!getfilesize(((SDATINFOSTRM*)((u8*)info + inforec7->nEntryOffset[i]))->fileID, &offset, &size))
            {
                printf("%d %d %d %s\n", ((SDATINFOSTRM*)((u8*)info + inforec7->nEntryOffset[i]))->fileID,
                       sdat_offset[sdat_iter] + offset, size, getfilename(7, i));
                if (mode == 1)
                {
                    FILE* f = fopen(getfilename(7, i), "wb");
                    fwrite(sdat + offset, 1, size, f);
                    fclose(f);
                }
            }
        }
    }
    else
    {
        for (i = 0; i < inforec0->nCount; i++)
        {
            int offset_sseq, size_sseq, offset_sbnk, size_sbnk;
            if (!getfilesize(((SDATINFOSSEQ*)((u8*)info + inforec0->nEntryOffset[i]))->fileID, &offset_sseq,
                             &size_sseq))
            { // this sseq entry is valid
                // really valid? based on caitsith2's fix
                /*
                            u32 fileoffset;
                            fseek(fnds,sdat_offset[sdat_iter]+offset_sseq+24,SEEK_SET); //SSEQ->nDataOffset
                            fread(&fileoffset,4,1,fnds);
                            fseek(fnds,sdat_offset[sdat_iter]+offset_sseq+fileoffset,SEEK_SET);
                            printf("%08x %08x\n",sdat_offset[sdat_iter]+offset_sseq,sdat_offset[sdat_iter]+fileoffset);
                            if(fgetc(fnds)!=0xfe)continue;
                */

                u16 sbnk = ((SDATINFOSSEQ*)((u8*)info + inforec0->nEntryOffset[i]))->bnk;
                if (sbnk < inforec2->nCount &&
                    !getfilesize(((SDATINFOSBNK*)((u8*)info + inforec2->nEntryOffset[sbnk]))->fileID, &offset_sbnk,
                                 &size_sbnk))
                { // refered sbnk entry is valid
                    int offset_swar[4] = { 0, 0, 0, 0 }, size_swar[4] = { 0, 0, 0, 0 };
                    u16 swar[4] = { 0xffff, 0xffff, 0xffff, 0xffff };
                    int count = 0;
                    for (j = 0; j < 4; j++)
                    {
                        swar[j] = ((SDATINFOSBNK*)((u8*)info + inforec2->nEntryOffset[sbnk]))->wa[j];
                        if (swar[j] < inforec3->nCount)
                        {
                            if (!getfilesize(((SDATINFOSWAR*)((u8*)info + inforec3->nEntryOffset[swar[j]]))->fileID,
                                             offset_swar + j, size_swar + j))
                                count++;
                        }
                    }
                    if (count)
                    {
                        sdat_entry++;
                        fputc(strlen(getfilename(0, i)), finfo);
                        fprintf(finfo, "%s", name); // getfilename(0,i));
                        // fputc(0,finfo);
                        offset_sseq += sdat_offset[sdat_iter];
                        fwrite(&offset_sseq, 4, 1, ftmp);
                        fwrite(&size_sseq, 4, 1, ftmp);
                        offset_sbnk += sdat_offset[sdat_iter];
                        fwrite(&offset_sbnk, 4, 1, ftmp);
                        fwrite(&size_sbnk, 4, 1, ftmp);
                        if (swar[0] != 0xffff)
                            offset_swar[0] += sdat_offset[sdat_iter];
                        fwrite(&offset_swar[0], 4, 1, ftmp);
                        fwrite(&size_swar[0], 4, 1, ftmp);
                        if (swar[1] != 0xffff)
                            offset_swar[1] += sdat_offset[sdat_iter];
                        fwrite(&offset_swar[1], 4, 1, ftmp);
                        fwrite(&size_swar[1], 4, 1, ftmp);
                        if (swar[2] != 0xffff)
                            offset_swar[2] += sdat_offset[sdat_iter];
                        fwrite(&offset_swar[2], 4, 1, ftmp);
                        fwrite(&size_swar[2], 4, 1, ftmp);
                        if (swar[3] != 0xffff)
                            offset_swar[3] += sdat_offset[sdat_iter];
                        fwrite(&offset_swar[3], 4, 1, ftmp);
                        fwrite(&size_swar[3], 4, 1, ftmp);
                    }
                }
            }
        }
    }
    return 0;
}

// argh... spaghetti...
static char head[512];

// nitrofs expander by loveemu under MIT license
static u8* fntT;
static u8* fatT;
static int ndsEnumFilesDir(char* prefix, u16 dir_id)
{
    int res = 0;
    // char strbuf[256];

    // read header
    // uint32 fnt_offset = mget4l(&nds[NDSHD_OFFSET_FNT_OFFSET]);

    u32 finfo_ofs = 8 * (dir_id & 0xfff);
    u32 entry_start = read32(&fntT[finfo_ofs]);     // reference location of entry name
    u16 top_file_id = read16(&fntT[finfo_ofs + 4]); // file ID of top entry
    // u16 parent_id = read16(&fntT[finfo_ofs+6]);   // ID of parent directory or directory count (root)

    finfo_ofs = entry_start;

    u16 file_id = top_file_id;
    for (;; file_id++)
    {
        u8 entry_type_name_length = fntT[finfo_ofs];
        u8 name_length = entry_type_name_length & 127;
        bool entry_type_directory = (entry_type_name_length & 128) ? true : false;
        if (name_length == 0)
            break;

        char entry_name[128];
        memcpy(entry_name, &fntT[finfo_ofs + 1], name_length);
        entry_name[name_length] = '\0';
        finfo_ofs += 1 + name_length;
        if (entry_type_directory)
        {
            u16 dir_id = read16(&fntT[finfo_ofs]);
            finfo_ofs += 2;

            // strcpy(strbuf, prefix);
            // strcat(strbuf, entry_name);
            // strcat(strbuf, "/");
            res = ndsEnumFilesDir("" /*strbuf*/, dir_id);
            if (res)
                return res;
        }
        else
        {
            u32 offset = read32(fatT + 8 * file_id);
            u32 size = read32(fatT + 8 * file_id + 4) - read32(fatT + 8 * file_id);
            if (size < 512)
            { /*fprintf(stderr,"too short\n");*/
                continue;
            }
            // printf("%08x %d %s\n",offset,size,entry_name);
            if (1
                // strlen(entry_name)>4&&(
                //	!strcasecmp(entry_name+strlen(entry_name)-5,".sdat")
                //	//|| !strcasecmp(entry_name+strlen(entry_name)-5,".smap")
                //)
            )
            { // argh should check every entry? damn
                fseek(fnds, offset, SEEK_SET);
                fread(head, 1, 512, fnds);
                fseek(fnds, offset, SEEK_SET);
                SDATHEADER* header = (SDATHEADER*)head;
                if (!memcmp(header->file.type, "SDAT", 4) && header->file.magic == 0x0100feff &&
                    // header->file.nFileSize==size           && //can be bad with garbaged nitrofs (like FF4(E))
                    header->file.nBlock > 2)
                {
                    sdat_offset[sdat_count] = offset;
                    sdat_size[sdat_count] = header->file.nFileSize;
                    sdat_head_size[sdat_count] = header->nFileSize;
                    sdat_count++;
                }
            }
        }
    }
    return res;
}

int sdatexpand(const int argc, const char** argv)
{
    // int ret=0; // lol?

    if (argc < 3)
    {
        fprintf(stderr, "SDATExpand: SDAT parser / NitroMusicPlayer info generater\n"
                        "List:    sdatexpand l name.nds\n"
                        "Extract: sdatexpand [ex] name.nds [DIR]\n"
                        "SPS:     [Win] X: (X is your drive) [Unix] chdir /path/to/volume\n"
                        "         sdatexpand i path/to/NDS path/to/name.sps\n");
        return -1;
    }

    mode = -1;
    if (argv[1][0] == 'l' || argv[1][0] == 'L')
        mode = 0;
    if (argv[1][0] == 'e' || argv[1][0] == 'E')
        mode = 1;
    if (argv[1][0] == 'x' || argv[1][0] == 'X')
        mode = 1;
    if (argv[1][0] == 's' || argv[1][0] == 'S')
        mode = 2;
    if (mode > 2)
    {
        fprintf(stderr, "cmd = [lexs]\n");
        return -1;
    }

    if (argc > 3)
    {
        if (mode == 1 && chdir(argv[3]))
        {
            fprintf(stderr, "chdir failed\n");
            return -1;
        }
    }
    if (mode == 2 && argc < 4)
    {
        fprintf(stderr, "specify sps name\n");
        return -1;
    }

    fnds = fopen(argv[2], "rb");
    if (!fnds)
    {
        fprintf(stderr, "fopen failed\n");
        return -1;
    }
    u32 size = filelength(fileno(fnds));
    if (size < 512)
    {
        fprintf(stderr, "too short\n");
        return -1;
    }
    fread(head, 1, 512, fnds);

    // enum sdats
    sdat_count = 0;
    memset(sdat_offset, 0, sizeof(sdat_offset));
    SDATHEADER* header = (SDATHEADER*)head;
    if (!memcmp(header->file.type, "SDAT", 4) && header->file.magic == 0x0100feff &&
        // header->file.nFileSize==size         &&
        header->file.nBlock > 2)
    {
        sdat_offset[sdat_count] = 0;
        sdat_size[sdat_count] = header->file.nFileSize;
        sdat_head_size[sdat_count] = header->nFileSize;
        sdat_count++;
    }
    else
    {
        // try nitrofs
        // fprintf(stderr,"trying nitrofs mode.\n");
        u32 fntT_offset = read32(head + 0x40), fntT_size = read32(head + 0x44);
        u32 fatT_offset = read32(head + 0x48), fatT_size = read32(head + 0x4c);
        if (fntT_offset + fntT_size < size && fatT_offset + fatT_size < size)
        {
            fntT = (u8*)malloc(fntT_size);
            fseek(fnds, fntT_offset, SEEK_SET);
            fread(fntT, 1, fntT_size, fnds);
            fatT = (u8*)malloc(fatT_size);
            fseek(fnds, fatT_offset, SEEK_SET);
            fread(fatT, 1, fatT_size, fnds);
            ndsEnumFilesDir("", 0xf000);
            free(fntT);
            free(fatT);
        }
    }
    if (!sdat_count)
    {
        fprintf(stderr, "not SDAT\n");
        fclose(fnds);
        return -1;
    }

    if (mode == 2)
    {
        ftmp = fopen(TMPFILE, "wb");
        finfo = fopen(argv[3], "wb");
        if (!finfo)
        {
            fprintf(stderr, "cannot open %s\n", argv[3]);
            return -1;
        }
        int i;
        for (i = 0; i < 4; i++)
            fputc(0, finfo);

#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
        {
            char utf8[768];
            {
                wchar_t x[256];
                MultiByteToWideChar(CP_ACP, 0, argv[2], -1, x, 256);
                WideCharToMultiByte(CP_UTF8, 0, x, -1, utf8, 768, NULL, NULL);
            }
            u32 name_offset = 13 + 1 + strlen(utf8);
            fwrite(&name_offset, 4, 1, finfo);
            for (i = 0; i < 4; i++)
                fputc(0, finfo); // dummy sseq_data_offset
            fputc(1 + strlen(utf8), finfo);
            fputc('/', finfo);
            fputs(utf8, finfo);
        }
#else
        u32 name_offset = 13 + 1 + strlen(argv[2]);
        fwrite(&name_offset, 4, 1, finfo);
        for (i = 0; i < 4; i++)
            fputc(0, finfo); // dummy sseq_data_offset
        fputc(1 + strlen(argv[2]), finfo);
        fputc('/', finfo);
        fputs(argv[2], finfo);
#endif
    }
    if (!isatty(fileno(stdout)))
        fprintf(stdout, "/%s\n", argv[2]);

    sdat_iter = 0;
    for (; sdat_iter < sdat_count; sdat_iter++)
    {
        if (!isatty(fileno(stdout)))
            fprintf(stdout, "%d %d\n", sdat_offset[sdat_iter], sdat_size[sdat_iter]);

        fseek(fnds, sdat_offset[sdat_iter], SEEK_SET);
        sdat = (u8*)malloc(mode == 1 ? sdat_size[sdat_iter] : sdat_head_size[sdat_iter]);
        if (!sdat)
        {
            fprintf(stderr, "malloc failed\n");
            return -1;
        }
        fread(sdat, 1, mode == 1 ? sdat_size[sdat_iter] : sdat_head_size[sdat_iter], fnds);
        /*ret=*/parseSDAT(); // doesn't return even if error
        free(sdat);
    }
    fclose(fnds);

    if (mode == 2)
    {
        fclose(ftmp);
        if (!sdat_entry)
        {
            fprintf(stderr, "SDAT doesn't have valid entry (perhaps strm)\n");
            fclose(finfo);
            remove(argv[3]);
            remove(TMPFILE);
            return 1;
        }
        // int pad=align4(ftell(finfo))-ftell(finfo);
        // for(;pad;pad--)fputc(0,finfo);
        int info_offset = ftell(finfo);
        ftmp = fopen(TMPFILE, "rb");
        int size = filelength(fileno(ftmp));
        for (; size; size -= min(size, BUFLEN))
        {
            fread(buf, 1, min(size, BUFLEN), ftmp);
            fwrite(buf, 1, min(size, BUFLEN), finfo);
        }
        fclose(ftmp);
        remove(TMPFILE);
        fseek(finfo, 0, SEEK_SET);
        fwrite(&sdat_entry, 4, 1, finfo);
        fseek(finfo, 8, SEEK_SET);
        fwrite(&info_offset, 4, 1, finfo);
        fclose(finfo);
    }
    return 0;
}
