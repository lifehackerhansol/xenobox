#include "../xenobox.h"
#include "../lib/hc128_ecrypt-sync.h"

// decievo - decode boot.ievo using RC4/HC-128 cipher.

/* Copyright (c) 1996 Pekka Pessi. All rights reserved.
 *
 * This source code is provided for unrestricted use. Users may copy or
 * modify this source code without charge.
 *
 * THIS SOURCE CODE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND
 * INCLUDING THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE
 * PRACTICE.
 */
typedef struct rc4_key
{
    unsigned char state[256];
    unsigned char x;
    unsigned char y;
} rc4_key;

#define swap_byte(x, y) \
    t = *(x);           \
    *(x) = *(y);        \
    *(y) = t

void rc4_prepare_key(unsigned char* key_data_ptr, int key_data_len, rc4_key* key)
{
    // int i;
    unsigned char t;
    // unsigned char swapByte;
    unsigned char index1;
    unsigned char index2;
    unsigned char* state;
    int counter;

    state = &key->state[0];
    for (counter = 0; counter < 256; counter++)
        state[counter] = counter;
    key->x = 0;
    key->y = 0;
    index1 = 0;
    index2 = 0;
    for (counter = 0; counter < 256; counter++)
    {
        index2 = (key_data_ptr[index1] + state[counter] + index2) % 256;
        swap_byte(&state[counter], &state[index2]);
        index1 = (index1 + 1) % key_data_len;
    }
}

void rc4(unsigned char* buffer_ptr, int buffer_len, rc4_key* key)
{
    unsigned char t;
    unsigned char x;
    unsigned char y;
    unsigned char* state;
    unsigned char xorIndex;
    int counter;

    x = key->x;
    y = key->y;
    state = &key->state[0];
    for (counter = 0; counter < buffer_len; counter++)
    {
        x = (x + 1) % 256;
        y = (state[x] + y) % 256;
        swap_byte(&state[x], &state[y]);
        xorIndex = (state[x] + state[y]) % 256;
        buffer_ptr[counter] ^= state[xorIndex];
    }
    key->x = x;
    key->y = y;
}

extern u8 ndshead[512];
static const u8 null256[256] = { 0 };

// thanks a lot, zorgluf!
// http://www.teamcyclops.com/forum/showpost.php?p=93653&postcount=24
static const char* _key = "c15c09d26939def94b2c110d6ffed971";
static const char* _iv_header = "3847D9EAC5B999457162C6E74F20420A";
// static const char *_iv_arm9 =   "6D78EBD08243DF63800BA2F00549A18F";
// static const char *_iv_arm7 =   "B2D75636E1F11C1315E06CA590E9F10F";

static int txt2bin(const char* src, u8* dst)
{
    int i = 0;
    for (; i < 64; i++)
    {
        unsigned char src0 = src[2 * i];
        if (!src0 || src0 == ' ' || src0 == '\t' || src0 == '\r' || src0 == '\n' || src0 == '#' || src0 == ';' ||
            src0 == '\'' || src0 == '"')
            break;
        if (0x60 < src0 && src0 < 0x7b)
            src0 -= 0x20;
        // if(!( isdigit(src0)||(0x40<src0&&src0<0x47) )){fprintf(stderr,"Invalid character %c\n",src0);exit(-1);}
        src0 = isdigit(src0) ? (src0 - '0') : (src0 - 55);

        unsigned char src1 = src[2 * i + 1];
        if (0x60 < src1 && src1 < 0x7b)
            src1 -= 0x20;
        // if(!( isdigit(src1)||(0x40<src1&&src1<0x47) )){fprintf(stderr,"Invalid character %c\n",src1);exit(-1);}
        src1 = isdigit(src1) ? (src1 - '0') : (src1 - 55);
        dst[i] = (src0 << 4) | src1;
        // fprintf(stderr,"%02X",dst[i]);
    }
    // if(i!=16){printf("key/iv length must be 16bytes in hex\n");exit(-1);}
    return i;
    // puts("");
}

static u8 head[512];

int decievo(const int argc, const char** argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "decievo boot.ievo ievo.nds [ievo.head]\n");
        return 1;
    }
    FILE* in = fopen(argv[1], "rb");
    if (!in)
    {
        fprintf(stderr, "fopen %s failed\n", argv[1]);
        return 2;
    }
    FILE* out = fopen(argv[2], "wb");
    if (!out)
    {
        fprintf(stderr, "fopen %s failed\n", argv[2]);
        return 2;
    }

    FILE* f_head;
    if (argc > 3 && (f_head = fopen(argv[3], "wb")))
    { // decrypt first area
        u32 offset;
        fseek(in, filelength(fileno(in)) - 4, SEEK_SET);
        fread(&offset, 4, 1, in);
        offset ^= 0x696D6520;
        offset -= 0x2000;
        // fprintf(stderr,"head offset: %08x\n",offset);
        fseek(in, offset, SEEK_SET);
        fread(buf, 1, 0x2000, in);
        rc4_key rc4ctx;
        rc4_prepare_key(buf, 16, &rc4ctx);
        rc4(buf + 16, 0x1ff0, &rc4ctx);
        fwrite(buf + 16, 1, 0x1ff0, f_head);
        fclose(f_head);
    }

    u8 key[16], iv[16];
    txt2bin(_key, key);
    txt2bin(_iv_header, iv);

    ECRYPT_ctx ctx;
    ECRYPT_keysetup(&ctx, key, 128, 128);
    ECRYPT_ivsetup(&ctx, iv);

    fseek(in, 0x80, SEEK_SET);
    fread(head, 1, 512, in);
    ECRYPT_process_bytes(0, &ctx, head, head,
                         512); // first arg is 0:encrypt, 1:decrypt. but as synmetric, we don't care.

    u32 o9 = read32(head + 0x08);
    u32 r9 = read32(head + 0x0c);
    u32 a9 = read32(head + 0x10);
    u32 l9 = read32(head + 0x14);

    u32 o7 = read32(head + 0x1c);
    u32 r7 = read32(head + 0x20);
    u32 a7 = read32(head + 0x24);
    u32 l7 = read32(head + 0x28);

    // fwrite(head,1,512,stdout);
    // iv_arm9 = head+0xb0
    // iv_arm7 = head+0xc0

    u32 pad9 = 0x100 - (l9 & 0xff);
    if (pad9 == 0x100)
        pad9 = 0;
    u32 pad7 = 0x100 - (l7 & 0xff);
    if (pad7 == 0x100)
        pad7 = 0;

    memcpy(buf, ndshead, 512);
    write32(buf + 0x24, r9);
    write32(buf + 0x28, a9);
    write32(buf + 0x2c, l9 /*+pad9*/);
    write32(buf + 0x30, l9 + pad9 + 0x200);
    write32(buf + 0x34, r7);
    write32(buf + 0x38, a7);
    write32(buf + 0x3c, l7 /*+pad7*/);
    write32(buf + 0x80, 0x200 + l9 + pad9 + l7 + pad7);
    write16(buf + 0x15e, swiCRC16(0xffff, buf, 0x15e));
    fwrite(buf, 1, 512, out);

    // txt2bin(_iv_arm9,iv);
    // ECRYPT_ivsetup(&ctx,iv);
    ECRYPT_ivsetup(&ctx, head + 0xb0);
    fseek(in, o9, SEEK_SET);
    u32 size = l9;
    for (; size; size -= min(size, BUFLEN))
    {
        int readlen = fread(buf, 1, min(size, BUFLEN), in);
        ECRYPT_process_bytes(0, &ctx, buf, buf, readlen);
        fwrite(buf, 1, readlen, out);
    }
    if (pad9)
        fwrite(null256, 1, pad9, out);

    // txt2bin(_iv_arm7,iv);
    // ECRYPT_ivsetup(&ctx,iv);
    ECRYPT_ivsetup(&ctx, head + 0xc0);
    fseek(in, o7, SEEK_SET);
    size = l7;
    for (; size; size -= min(size, BUFLEN))
    {
        int readlen = fread(buf, 1, min(size, BUFLEN), in);
        ECRYPT_process_bytes(0, &ctx, buf, buf, readlen);
        fwrite(buf, 1, readlen, out);
    }
    if (pad7)
        fwrite(null256, 1, pad7, out);

    fclose(out);

    return 0;
}
