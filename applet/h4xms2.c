static const unsigned char* key = (unsigned char*)                     // total 56bytes
    "\x1B\x01\xD0\x05\x03\x90\xD3\x05\x09\x00\x50\x01\x05\x00\x00\x1A" // key_rudolph (16bytes)
    "\x00\x00\xA0\xE3\x00\x20\xC3\xE7\x01\x00\x80\xE2"                 // interrude (12bytes)
    "\x04\x00\x50\xE3\xFB\xFF\xFF\xBA\x00\x50\xCC\xE5\x01\x10\x81\xE2\x04\x00\x51\xE3\xE8\xFF\xFF\x3A\x00\x00\xDC\xE5"; // key_normmatt (28bytes)

#include "../xenobox.h"

int h4xms2(const int argc, const char** argv)
{
    unsigned char* ys;
    unsigned size, ext;
    FILE* f;
    int flag = 0;

    // startup
    fprintf(stderr, "h4xms2 - overlay.dll will allow commercial roms\n");
    if (argc < 2)
    {
        fprintf(stderr, "specify path for overlay.dll\n");
        // fprintf(stderr,"use extra arg to use rudolph's patching\n");
        return 1;
    }
    ext = strlen(argv[1]);
    if (ext < 4 || argv[1][ext - 4] != '.')
    {
        fprintf(stderr, "file has to have extention such as .dll\n");
        return 1;
    }
    ext -= 3;

    // read
    f = fopen(argv[1], "rb");
    if (!f)
    {
        fprintf(stderr, "cannot open\n");
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
    fprintf(stderr, "h4xing overlay...\n");
    int i = 0x200, p = 0;
    for (; i < size - 56; i += 4)
    {
        if (!memcmp(ys + i, key, 56))
        {
            if (flag)
            {
                fprintf(stderr, "multiple hits (patch error)\n");
                free(ys);
                return -1;
            }
            flag = 1;
            p = i;
        }
    }
    if (p)
    {
        int q = p;
        // if(argc<3){ //Normmatt
        p = q + 52;
        fprintf(stderr, "Patched 0x%08X (Normmatt)\n", p);
        // if(ys[p+3]!=0xe5){fprintf(stderr,"incorrect detection (patch error)\n");free(ys);return -1;}
        ys[p] = 0x01, ys[p + 1] = 0x00, ys[p + 2] = 0xa0, ys[p + 3] = 0xe3;
        //}else{ //Rudolph
        p = q + 12;
        fprintf(stderr, "Patched 0x%08X (Rudolph)\n", p);
        // if(ys[p+3]!=0x1a){fprintf(stderr,"incorrect detection (patch error)\n");free(ys);return -1;}
        ys[p] = 0x00, ys[p + 1] = 0x00, ys[p + 2] = 0xa0, ys[p + 3] = 0xe3;
        //}
    }
    if (!flag)
    {
        fprintf(stderr, "no hits or already patched\n");
        free(ys);
        return -1;
    }

    // write
    {
        // char *n=(char*)malloc(strlen(argv[1])+1);
        // strcpy(n,argv[1]);
        // n[ext]='b';n[ext+1]='a';n[ext+2]='k';
        // rename(argv[1],n);
        // free(n);
    }
    f = fopen(argv[1], "wb");
    fwrite(ys, 1, size, f);
    fclose(f);
    free(ys);
    fprintf(stderr, "patch OK\n");
    return 0;
}
