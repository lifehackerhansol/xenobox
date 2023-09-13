#include "../xenobox.h"

static unsigned char* pbuf;
static void Phase1(const u8* t)
{
    int i = 0, j = 0;
    if (!memcmp(pbuf + 0x0c, "####", 4))
    {
        fprintf(stderr, "Header already decrypted\n");
        return;
    }
    for (; i < 0x200; i++)
        if (pbuf[i])
            // pbuf[i]=pbuf[i]^t[j++],j%=strlen(t);
            pbuf[i] = pbuf[i] != t[j] ? pbuf[i] ^ t[j] : pbuf[i], j++, j %= strlen((char*)t); // fixed in V2
    fprintf(stderr, "Header decrypted successfully\n");
}

static void Phase2(const u8* k1, const u8* k2, const int size)
{ // added in V3
    int i, j;
    if ((pbuf[0x203] & 0xf0) == 0xe0)
    {
        fprintf(stderr, "Not seems to need decrypting\n");
        return;
    }
    /*
        fprintf(stderr,"stage1=0x200-");
        for(j=0;;i++){
                if(!(i&3)&&!(*(int*)(pbuf+i)))break;
                pbuf[i]=pbuf[i]^k1[j++],j%=strlen(k1);
        }
        fprintf(stderr,"0x%x stage2=",i-1);
        for(;;i+=4)if(!(i&3)&&(*(int*)(pbuf+i)))break;
        fprintf(stderr,"0x%x-",i);
        for(j=0;;i++){
                if(!(i&3)&&!(*(int*)(pbuf+i)))break;
                pbuf[i]=pbuf[i]^k2[j++],j%=strlen(k2);
        }
        fprintf(stderr,"0x%x\n",i-1);
    */

    // V3a: more precise. This is preparation for my ak2loader.
    fprintf(stderr, "ARM9=0x%x-0x%x ", read32(pbuf + 0x20), read32(pbuf + 0x20) + read32(pbuf + 0x2c) - 1);
    i = read32(pbuf + 0x20);
    for (j = 0; i < read32(pbuf + 0x20) + read32(pbuf + 0x2c); i++)
    {
        pbuf[i] = pbuf[i] ^ k1[j++], j %= strlen((char*)k1);
    }
    fprintf(stderr, "ARM7=0x%x-0x%x\n", read32(pbuf + 0x30), read32(pbuf + 0x30) + read32(pbuf + 0x3c) - 1);
    i = read32(pbuf + 0x30);
    for (j = 0; i < read32(pbuf + 0x30) + read32(pbuf + 0x3c); i++)
    {
        pbuf[i] = pbuf[i] ^ k2[j++], j %= strlen((char*)k2);
    }
    fprintf(stderr, "Decrypted successfully\n");
}

int akaiodec_v4(const int argc, const char** argv)
{
    int i = 1; // 2;

    // V4: keychanged
    const u8* t = (u8*)"CuNt"; // argv[1];
    // These two keys are top secret! Don't tell this to anyone! Mongai-Fushutsu!
    const u8* akaiokey_arm9 = (u8*)"DoNt hAx";
    const u8* akaiokey_arm7 = (u8*)"My SHitz";

    /*
        const u8 *t="FUCK";//argv[1];
        //These two keys are top secret! Don't tell this to anyone! Mongai-Fushutsu!
        const u8 *akaiokey_arm9="\x82\x39\x48\x53\x63\x24\x86\x41";
        const u8 *akaiokey_arm7="\x85\x56\x34\x21\x12\x67\x35\x14";
    */

    FILE* f;
    struct stat st;

    fprintf(stderr, "AKAIO loader decode in C v4\n");
    if (argc < i + 1)
    {
        fprintf(stderr, "Usage: akaiodec loader...\n");
        msleep(1000);
        return -1;
    } // Currently KEY is "FUCK"
    // len=strlen(t);
    for (; i < argc; i++)
    {
        fprintf(stderr, "%s: ", argv[i]);
        f = fopen(argv[i], "rb+");
        if (!f)
        {
            fprintf(stderr, "Cannot open\n");
            continue;
        }
        fstat(fileno(f), &st);
        if (st.st_size < 0x200)
        {
            fclose(f);
            fprintf(stderr, "Too small\n");
            continue;
        }
        pbuf = malloc(st.st_size);
        fread(pbuf, 1, st.st_size, f);
        rewind(f);
        fprintf(stderr, "Start decryption\n");

        fprintf(stderr, "Phase1: ");
        Phase1(t);

        fprintf(stderr, "Phase2: ");
        Phase2(akaiokey_arm9, akaiokey_arm7, st.st_size);

        fwrite(pbuf, 1, st.st_size, f);
        free(pbuf);
        fclose(f);
    }
    msleep(1000);
    return 0;
}
