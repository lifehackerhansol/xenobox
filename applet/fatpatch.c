/*
    fatpatch rev2 - fix the final cluster bug in gba_nds_fat

    flag:
    1==CheckDiskNDS
    2==MoonShell RVCT (say, iSakuReal and EZ5)
    0x100==DSorganize
*/

#include "../xenobox.h"

int fatpatch(const int argc, const char** argv)
{
    unsigned char* ys;
    unsigned size, ext;
    FILE* f;
    int flag = 0, _flag;

    // startup
    fprintf(stderr, "fatpatch rev2 - modified yspatch\n");
    if (argc < 2)
    {
        fprintf(stderr, "specify path for nds\n");
        return 1;
    }
    ext = strlen(argv[1]);
    if (ext < 4 || argv[1][ext - 4] != '.')
    {
        fprintf(stderr, "file has to have extention such as .nds or .dat\n");
        return 1;
    }
    ext -= 3;

    // read
    f = fopen(argv[1], "rb");
    if (!f)
    {
        fprintf(stderr, "cannot open nds\n");
        return 2;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size > 0x2000000)
    {
        fprintf(stderr, "too big\n");
        fclose(f);
        return 3;
    }
    ys = (unsigned char*)malloc(size);
    if (!ys)
    {
        fprintf(stderr, "cannot alloc memory\n");
        fclose(f);
        return 4;
    }
    fread(ys, 1, size, f);
    fclose(f);

    // patch
    fprintf(stderr, "Patch phase1...\n");
    fprintf(stderr, "Trying ARM...\n");
    {
        int i = 0x200, p = 0;
        for (; i < size - 100; i += 4)
        {
            if (((ys[i + 3] == 0xe3 && ys[i + 2] == 0x50 && ys[i + 1] == 0x00 &&
                  ys[i] == 0x01 && // cmp r0,#0x1 // gcc prefers this
                  ys[i + 7] == 0x9a && ys[i + 6] == 0x00 && ys[i + 5] == 0x00 && ys[i + 4] == 0x02 // bls +0x10 (skip 3)
                  ) ||
                 (ys[i + 3] == 0xe3 && ys[i + 2] == 0x50 && ys[i + 1] == 0x00 &&
                  ys[i] == 0x02 && // cmp r0,#0x2 // RVCT prefers this
                  ys[i + 7] == 0x3a && ys[i + 6] == 0x00 && ys[i + 5] == 0x00 && ys[i + 4] == 0x02 // bcc +0x10 (skip 3)
                  )) &&
                ys[i + 11] == 0xe5 &&                                          // ldr ...
                ys[i + 15] == 0xe1 && ys[i + 14] == 0x50 && ys[i + 13] == 0x00 // cmp r0,r?
                //(ys[i+19]&0xf0)==0x30                                             // ldmccia(38) / bcc(3a) (return)
            )
            {
                if ((ys[i + 19] & 0xf0) != 0x30 && (ys[i + 19] & 0xf0) != 0x90)
                    continue;
                if (flag)
                {
                    fprintf(stderr, "multiple hits (patch error)\n");
                    free(ys);
                    return -1;
                }
                flag = ys[i] == 0x01 ? 1 : 2;
                p = i + 19;
            }
        }
        if (p)
        {
            fprintf(stderr, "%sPatched 0x%08X [%s]\n", (ys[p] & 0xf0) == 0x90 ? "Already " : "", p,
                    flag == 1 ? "CheckDiskNDS" : "MoonShell RVCT");
            ys[p] = 0x90 | (ys[p] & 0x0f); // ldmlsia / bls (return)
        }
    }

    fprintf(stderr, "Trying Thumb...\n");
    {
        int i = 0x200, p = 0;
        for (; i < size - 100; i += 2)
        {
            if (ys[i + 1] == 0x28 && ys[i] == 0x01 &&     // cmp r0,#0x1
                ys[i + 3] == 0xd9 && ys[i + 2] == 0x02 && // bcc +0x10 (skip 3)
                                                          // ldr ...
                ys[i + 7] == 0x42 && ys[i + 6] == 0x98 && // cmp r0,r3
                // ys[i+ 9]==0xd3                 && // bcc (return)
                ys[i + 11] == 0xf7 && ys[i + 10] == 0xff // bl (long)
            )
            {
                if (ys[i + 9] != 0xd3 && ys[i + 9] != 0xd9)
                    continue;
                if (flag)
                {
                    fprintf(stderr, "multiple hits (patch error)\n");
                    free(ys);
                    return -1;
                }
                flag = 0x100;
                p = i + 9;
            }
        }
        if (p)
        {
            fprintf(stderr, "%sPatched 0x%08X [%s]\n", ys[p] == 0xd9 ? "Already " : "", p, "libFATDragon");
            // if(ys[p]!=0x0a){fprintf(stderr,"incorrect detection (patch error)\n");free(ys);return -1;}
            ys[p] = 0xd9; // bls (return)
        }
    }

    fprintf(stderr, "Trying libfat (phase2)...\n");
    {
        int i = 0x200, p = 0, fix = 0;
        for (; i < size - 100; i += 2)
        {
            if (ys[i + 1] == 0xd1 &&          // bne ...
                (ys[i + 3] & 0xf0) == 0x60 && // str ...
                (ys[i + 5] & 0xf0) == 0x40 && // cmp r?,r?
                // ys[i+ 7]==0xd2                 && // bcs
                (ys[i + 9] & 0xf0) == 0x20 && ys[i + 8] == 0x01 && // cmp r?,#0x1
                ys[i + 11] == 0xd9                                 // bls ...
            )
            {
                if (ys[i + 6] - ys[i + 10] != 2 || (ys[i + 7] != 0xd2 && ys[i + 7] != 0xd8))
                    continue;
                if (flag)
                {
                    fprintf(stderr, "multiple hits (patch error)\n");
                    free(ys);
                    return -1;
                }
                flag = -1;
                p = i + 7;
                fix = 0xd8; // bcs -> bhi
            }
        }
        for (i = 0x200; i < size - 100; i += 2)
        {
            if (ys[i + 1] == 0xd1 &&                               // bne ...
                ((ys[i + 3] & 0xf0) == 0x40) &&                    // mov r?,r?
                ((ys[i + 5] & 0xf0) == 0x60) &&                    // str ...
                (ys[i + 7] & 0xf0) == 0x20 && ys[i + 6] == 0x01 && // cmp r?,#0x1
                ys[i + 9] == 0xd9 &&                               // bls ...
                (ys[i + 11] & 0xf0) == 0x40                        // cmp r?,r?
                                                                   // ys[i+13]==0xd9                  // bls
            )
            {
                if (ys[i + 8] - ys[i + 12] != 2 || (ys[i + 13] != 0xd9 && ys[i + 13] != 0xd3))
                    continue;
                if (flag)
                {
                    fprintf(stderr, "multiple hits (patch error)\n");
                    free(ys);
                    return -1;
                }
                flag = -1;
                p = i + 13;
                fix = 0xd3; // bls -> bcc
            }
        }
        if (p)
        {
            fprintf(stderr, "I hope libfat doesn't have phase1 issue.\n");
            fprintf(stderr, "%sPatched 0x%08X [%s]\n", ys[p] == fix ? "Already " : "", p,
                    p == i + 7 ? "legacy" : "1.4.x");
            // if(ys[p]!=0x0a){fprintf(stderr,"incorrect detection (patch error)\n");free(ys);return -1;}
            ys[p] = fix; // bls (return)
        }
    }

    if (!flag)
    {
        fprintf(stderr, "no hits\n");
        free(ys);
        return -1;
    }
    if (flag == -1)
        goto write;
    _flag = flag;
    flag = 0;

    fprintf(stderr, "Patch phase2...\n");
    if (_flag == 1)
    {
        int i = 0x200, p = 0;
        for (; i < size - 100; i += 4)
        {
            if (ys[i + 3] == 0xe3 && ys[i + 1] == 0x00 && ys[i] == 0x01 && // cmp r?,#0x1 // gcc prefers this
                ys[i + 7] == 0x9a && ys[i + 6] == 0x00 && ys[i + 5] == 0x00 && ys[i + 4] == 0x01 && // bls +0xc (skip 2)
                ys[i + 11] == 0xe1 && ys[i + 9] == 0x00 &&                                          // cmp r?,r?
                ys[i + 14] == 0x00 && ys[i + 13] == 0x00                                            // bcc (write)
            )
            {
                if ((ys[i + 15] & 0xf0) != 0x30 && (ys[i + 15] & 0xf0) != 0x90)
                    continue;
                if (flag)
                {
                    fprintf(stderr, "multiple hits (patch error)\n");
                    free(ys);
                    return -1;
                }
                flag = 1;
                p = i + 15;
            }
        }
        for (i = 0x200; i < size - 100; i += 4)
        {
            if (ys[i + 3] == 0xe3 && ys[i + 1] == 0x00 && ys[i] == 0x01 && // cmp r?,#0x1 // gcc prefers this
                ys[i + 7] == 0x9a && ys[i + 6] == 0x00 && ys[i + 5] == 0x00 &&
                ys[i + 4] == 0x04 &&                        // bls +0x18 (skip 5)
                ys[i + 11] == 0xe5 &&                       // ldr ...
                ys[i + 15] == 0xe1 && ys[i + 13] == 0x00 && // cmp r?,r?
                ys[i + 26] == 0x00 && ys[i + 25] == 0x00    // bcc (write)
            )
            {
                if ((ys[i + 27] & 0xf0) != 0x30 && (ys[i + 27] & 0xf0) != 0x90)
                    continue;
                if (flag)
                {
                    fprintf(stderr, "multiple hits (patch error)\n");
                    free(ys);
                    return -1;
                }
                flag = 1;
                p = i + 27;
            }
        }
        if (p)
        {
            fprintf(stderr, "%sPatched 0x%08X\n", (ys[p] & 0xf0) == 0x90 ? "Already " : "", p);
            ys[p] = 0x90 | (ys[p] & 0x0f); // bls (write)
        }
    }
    if (_flag == 2)
    {
        int i = 0x200, p = 0;
        for (; i < size - 100; i += 4)
        {
            if (ys[i + 3] == 0xe3 && ys[i + 1] == 0x00 && ys[i] == 0x02 && // cmp r?,#0x2 // RVCT prefers this
                ys[i + 7] == 0x3a && ys[i + 6] == 0x00 && ys[i + 5] == 0x00 &&
                ys[i + 4] == 0x04 &&                     // bcc +0x18 (skip 5)
                ys[i + 11] == 0xe5 &&                    // ldr ...
                ys[i + 15] == 0xe1 && ys[i + 13] == 0x00 // cmp r?,r?
            )
            {
                if (((ys[i + 19] & 0xf0) != 0x30 && (ys[i + 19] & 0xf0) != 0x90) ||
                    ((ys[i + 23] & 0xf0) != 0x30 && (ys[i + 23] & 0xf0) != 0x90) ||
                    ((ys[i + 27] & 0xf0) != 0x30 && (ys[i + 27] & 0xf0) != 0x90))
                    continue;
                if (flag)
                {
                    fprintf(stderr, "multiple hits (patch error)\n");
                    free(ys);
                    return -1;
                }
                flag = 2;
                p = i + 19;
            }
        }
        for (i = 0x200; i < size - 100; i += 4)
        {
            if (ys[i + 3] == 0xe3 && ys[i + 1] == 0x00 && ys[i] == 0x02 && // cmp r?,#0x2 // RVCT prefers this
                ys[i + 7] == 0x3a && ys[i + 6] == 0x00 && ys[i + 5] == 0x00 &&
                ys[i + 4] == 0x03 &&                    // bcc +0x18 (skip 5)
                ys[i + 11] == 0xe1 && ys[i + 9] == 0x00 // cmp r?,r?
            )
            {
                if (((ys[i + 15] & 0xf0) != 0x30 && (ys[i + 15] & 0xf0) != 0x90) ||
                    ((ys[i + 19] & 0xf0) != 0x30 && (ys[i + 19] & 0xf0) != 0x90) ||
                    ((ys[i + 23] & 0xf0) != 0x30 && (ys[i + 23] & 0xf0) != 0x90))
                    continue;
                if (flag)
                {
                    fprintf(stderr, "multiple hits (patch error)\n");
                    free(ys);
                    return -1;
                }
                flag = 2;
                p = i + 15;
            }
        }
        if (p)
        {
            fprintf(stderr, "%sPatched 0x%08X\n", (ys[p] & 0xf0) == 0x90 ? "Already " : "", p);
            ys[p] = 0x90 | (ys[p] & 0x0f); //*ls
            fprintf(stderr, "%sPatched 0x%08X\n", (ys[p + 4] & 0xf0) == 0x90 ? "Already " : "", p + 4);
            ys[p + 4] = 0x90 | (ys[p + 4] & 0x0f); //*ls
            fprintf(stderr, "%sPatched 0x%08X\n", (ys[p + 8] & 0xf0) == 0x90 ? "Already " : "", p + 8);
            ys[p + 8] = 0x90 | (ys[p + 8] & 0x0f); //*ls
        }
    }
    if (_flag == 0x100)
    {
        int i = 0x200, p = 0;
        for (; i < size - 100; i += 2)
        {
            if (ys[i + 1] == 0x2d && ys[i] == 0x01 &&     // cmp r5,#0x1
                ys[i + 3] == 0xd9 && ys[i + 2] == 0x07 && // bcc +0x10 (skip 8)
                                                          // ldr ...
                ys[i + 7] == 0x68 && ys[i + 6] == 0x1b && // ldr r3,[r3]
                ys[i + 9] == 0x42 && ys[i + 8] == 0x9d    // cmp r5,r3
                                                          // ys[i+11]==0xd2                 && // bcs (not write)
                // ys[i+13]==0xf7&&ys[i+12]==0xff    // bl (long)
            )
            {
                if (ys[i + 11] != 0xd2 && ys[i + 11] != 0xd8)
                    continue;
                if (flag)
                {
                    fprintf(stderr, "multiple hits (patch error)\n");
                    free(ys);
                    return -1;
                }
                flag = 0x100;
                p = i + 11;
            }
        }
        if (p)
        {
            fprintf(stderr, "%sPatched 0x%08X\n", ys[p] == 0xd8 ? "Already " : "", p);
            // if(ys[p]!=0x0a){fprintf(stderr,"incorrect detection (patch error)\n");free(ys);return -1;}
            ys[p] = 0xd8; // bhi (write)
        }
    }

    if (!flag)
    {
        fprintf(stderr, "no hits\n");
        free(ys);
        return -1;
    }

write:
    // write
#if 0
  {
    char *n=(char*)malloc(strlen(argv[1])+1);
    strcpy(n,argv[1]);
    n[ext]='b';n[ext+1]='a';n[ext+2]='k';
    rename(argv[1],n);
    free(n);
  }
#endif
    f = fopen(argv[1], "wb");
    fwrite(ys, 1, size, f);
    fclose(f);
    free(ys);
    fprintf(stderr, "patch OK\n");
    return 0;
}
