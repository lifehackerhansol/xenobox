#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
#include "../libprism.h"
#else
#include "../xenobox.h"
#endif

/*
    ndstool encryption.c
    ToDo: refactoring

    __attribute__((noinline)) : -O3 has some issue (inline-ize too much)
*/

// #define NOINLINE __attribute__((noinline))
#define NOINLINE

#define ROMTYPE_HOMEBREW   0
#define ROMTYPE_MULTIBOOT  1
#define ROMTYPE_NDSDUMPED  2 // decrypted secure area
#define ROMTYPE_ENCRSECURE 3
#define ROMTYPE_MASKROM    4 // unknown layout

#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
#define PRINTF _consolePrintf2
#else
#include <stdarg.h>
static int PRINTF(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vfprintf(stderr, format, args);
    va_end(args);
    return ret;
}
#endif

extern const u8* encr_data;

struct Header
{
    char title[0xC];
    char gamecode[0x4];
    char makercode[2];
    unsigned char unitcode;       // product code. 0 = Nintendo DS
    unsigned char devicetype;     // device code. 0 = normal
    unsigned char devicecap;      // device size. (1<<n Mbit)
    unsigned char reserved1[0x9]; // 0x015..0x01D
    unsigned char romversion;
    unsigned char reserved2;      // 0x01F
    unsigned int arm9_rom_offset; // points to libsyscall and rest of ARM9 binary
    unsigned int arm9_entry_address;
    unsigned int arm9_ram_address;
    unsigned int arm9_size;
    unsigned int arm7_rom_offset;
    unsigned int arm7_entry_address;
    unsigned int arm7_ram_address;
    unsigned int arm7_size;
    unsigned int fnt_offset;
    unsigned int fnt_size;
    unsigned int fat_offset;
    unsigned int fat_size;
    unsigned int arm9_overlay_offset;
    unsigned int arm9_overlay_size;
    unsigned int arm7_overlay_offset;
    unsigned int arm7_overlay_size;
    unsigned int rom_control_info1; // 0x00416657 for OneTimePROM
    unsigned int rom_control_info2; // 0x081808F8 for OneTimePROM
    unsigned int banner_offset;
    unsigned short secure_area_crc;
    unsigned short rom_control_info3;    // 0x0D7E for OneTimePROM
    unsigned int offset_0x70;            // magic1 (64 bit encrypted magic code to disable LFSR)
    unsigned int offset_0x74;            // magic2
    unsigned int offset_0x78;            // unique ID for homebrew
    unsigned int offset_0x7C;            // unique ID for homebrew
    unsigned int application_end_offset; // rom size
    unsigned int rom_header_size;
    unsigned int offset_0x88; // reserved... ?
    unsigned int offset_0x8C;

    // reserved
    unsigned int offset_0x90;
    unsigned int offset_0x94;
    unsigned int offset_0x98;
    unsigned int offset_0x9C;
    unsigned int offset_0xA0;
    unsigned int offset_0xA4;
    unsigned int offset_0xA8;
    unsigned int offset_0xAC;
    unsigned int offset_0xB0;
    unsigned int offset_0xB4;
    unsigned int offset_0xB8;
    unsigned int offset_0xBC;

    unsigned char logo[156]; // character data
    unsigned short logo_crc;
    unsigned short header_crc;

    // 0x160..0x17F reserved
    unsigned int offset_0x160;
    unsigned int offset_0x164;
    unsigned int offset_0x168;
    unsigned int offset_0x16C;
    unsigned char zero[0x40];
    u8 regionmask;
    u8 h1B1[0x0E];
    u8 appflags;
    unsigned int dsi9_rom_offset;
    unsigned int dsi9_entry_address;
    unsigned int dsi9_ram_address;
    unsigned int dsi9_size;
    unsigned int dsi7_rom_offset;
    unsigned int dsi7_entry_address;
    unsigned int dsi7_ram_address;
    unsigned int dsi7_size;
    unsigned int offset_0x1E0;
    unsigned int offset_0x1E4;
    unsigned int dsi_region_start;
    unsigned int dsi_region_size;
    unsigned int hash1_start;
    unsigned int hash1_size;
    unsigned int hash2_start;
    unsigned int hash2_size;
};

static struct Header header;
static FILE* fNDS;

static int DetectRomType()
{
    fseek(fNDS, 0x4000, SEEK_SET);
    unsigned int data[3];
    fread(data, 1, sizeof(data), fNDS);
    if (header.arm9_rom_offset < 0x4000)
        return ROMTYPE_HOMEBREW;
    if (data[0] == 0x00000000 && data[1] == 0x00000000)
        return ROMTYPE_MULTIBOOT;
    if (data[0] == 0xE7FFDEFF && data[1] == 0xE7FFDEFF)
        return ROMTYPE_NDSDUMPED;
    fseek(fNDS, 0x1000, SEEK_SET);
    int i = 0x1000;
    for (; i < 0x4000; i++)
        if (fgetc(fNDS))
            return ROMTYPE_MASKROM; // found something odd ;)
    return ROMTYPE_ENCRSECURE;
}

u32 card_hash[0x412];
// static int cardheader_devicetype = 0;
static u32 global3_x00, global3_x04; // RTC value
static u32 global3_rand1;
static u32 global3_rand3;

static NOINLINE u32 lookup(u32* magic, u32 v)
{
    u32 a = (v >> 24) & 0xFF;
    u32 b = (v >> 16) & 0xFF;
    u32 c = (v >> 8) & 0xFF;
    u32 d = (v >> 0) & 0xFF;

    a = magic[a + 18 + 0];
    b = magic[b + 18 + 256];
    c = magic[c + 18 + 512];
    d = magic[d + 18 + 768];

    return d + (c ^ (b + a));
}

NOINLINE void enc_encrypt(u32* magic, u32* arg1, u32* arg2)
{
    u32 a, b, c;
    a = *arg1;
    b = *arg2;
    int i = 0;
    for (; i < 16; i++)
    {
        c = magic[i] ^ a;
        a = b ^ lookup(magic, c);
        b = c;
    }
    *arg2 = a ^ magic[16];
    *arg1 = b ^ magic[17];
}

NOINLINE void enc_decrypt(u32* magic, u32* arg1, u32* arg2)
{
    u32 a, b, c;
    a = *arg1;
    b = *arg2;
    int i = 17;
    for (; i > 1; i--)
    {
        c = magic[i] ^ a;
        a = b ^ lookup(magic, c);
        b = c;
    }
    *arg1 = b ^ magic[0];
    *arg2 = a ^ magic[1];
}

#if 0
void encrypt(u32 *magic, u64 &cmd){
	encrypt(magic, (u32 *)&cmd + 1, (u32 *)&cmd + 0);
}

void decrypt(u32 *magic, u64 &cmd){
	decrypt(magic, (u32 *)&cmd + 1, (u32 *)&cmd + 0);
}
#endif

NOINLINE void enc_update_hashtable(u32* magic, u8* arg1, u32 modulo)
{
    int i, j;
    for (j = 0; j < 18; j++)
    {
        u32 r3 = 0;
        for (i = 0; i < 4; i++)
        {
            r3 <<= 8;
            r3 |= arg1[(j * 4 + i) % modulo];
        }
        magic[j] ^= r3;
    }

    u32 tmp1 = 0;
    u32 tmp2 = 0;
    for (i = 0; i < 18; i += 2)
    {
        enc_encrypt(magic, &tmp1, &tmp2);
        magic[i + 0] = tmp1;
        magic[i + 1] = tmp2;
    }
    for (i = 0; i < 0x400; i += 2)
    {
        enc_encrypt(magic, &tmp1, &tmp2);
        magic[i + 18 + 0] = tmp1;
        magic[i + 18 + 1] = tmp2;
    }
}

u32 arg2[3];

NOINLINE void enc_init2(u32* magic, u32* a, u32 modulo)
{
    enc_encrypt(magic, a + 2, a + 1);
    enc_encrypt(magic, a + 1, a + 0);
    enc_update_hashtable(magic, (u8*)a, modulo);
}

NOINLINE void enc_init1(u32 cardheader_gamecode, int level, u32 modulo)
{
    memcpy(card_hash, encr_data, 4 * (1024 + 18));
    arg2[0] = cardheader_gamecode;
    arg2[1] = cardheader_gamecode >> 1;
    arg2[2] = cardheader_gamecode << 1;
    enc_init2(card_hash, arg2, modulo);
    if (level >= 2)
        enc_init2(card_hash, arg2, modulo);
}

static NOINLINE void init0(u32 cardheader_gamecode)
{
    enc_init1(cardheader_gamecode, 2, 8);
    enc_encrypt(card_hash, (u32*)&global3_x04, (u32*)&global3_x00);
    global3_rand1 = global3_x00 ^ global3_x04; // more RTC
    global3_rand3 = global3_x04 ^ 0x0380FEB2;
    enc_encrypt(card_hash, (u32*)&global3_rand3, (u32*)&global3_rand1);
}

// ARM9 decryption check values
#define MAGIC30 0x72636E65
#define MAGIC34 0x6A624F79

static int decrypt_arm9(u32 cardheader_gamecode, unsigned char* data)
{
    u32* p = (u32*)data;

    enc_init1(cardheader_gamecode, 2, 8);
    enc_decrypt(card_hash, p + 1, p);
    arg2[1] <<= 1;
    arg2[2] >>= 1;
    enc_init2(card_hash, arg2, 8);
    enc_decrypt(card_hash, p + 1, p);

    if (p[0] != MAGIC30 || p[1] != MAGIC34)
    {
        PRINTF("Decryption failed!\n");
        return 1;
    }

    *p++ = 0xE7FFDEFF;
    *p++ = 0xE7FFDEFF;
    u32 size = 0x800 - 8;
    while (size > 0)
    {
        enc_decrypt(card_hash, p + 1, p);
        p += 2;
        size -= 8;
        // PRINTF("Decrypting %4d / 2048\r",0x800-size);
    }
    return 0;
}

static int encrypt_arm9(u32 cardheader_gamecode, unsigned char* data)
{
    u32* p = (u32*)data;
    if (p[0] != 0xE7FFDEFF || p[1] != 0xE7FFDEFF)
    {
        PRINTF("Encryption failed!\n");
        return 1;
    }
    p += 2;

    enc_init1(cardheader_gamecode, 2, 8);

    arg2[1] <<= 1;
    arg2[2] >>= 1;

    enc_init2(card_hash, arg2, 8);

    u32 size = 0x800 - 8;
    while (size > 0)
    {
        enc_encrypt(card_hash, p + 1, p);
        p += 2;
        size -= 8;
        // PRINTF("Encrypting %4d / 2048\r",0x800-size);
    }

    // place header

    p = (u32*)data;
    p[0] = MAGIC30;
    p[1] = MAGIC34;
    enc_encrypt(card_hash, p + 1, p);
    enc_init1(cardheader_gamecode, 2, 8);
    enc_encrypt(card_hash, p + 1, p);
    return 0;
}

static unsigned short CalcHeaderCRC(struct Header header)
{
    return swiCRC16(0xffff, (unsigned char*)&header, 0x15E);
}

static unsigned short CalcLogoCRC(struct Header header)
{
    return swiCRC16(0xffff, (unsigned char*)&header + 0xC0, 156);
}

static unsigned char data[0x4000];
NOINLINE void EnDecryptSecureArea(const char* ndsfilename, char endecrypt_option)
{
    fNDS = fopen(ndsfilename, "r+b");
    if (!fNDS)
    {
        PRINTF("Cannot open file '%s'.\n", ndsfilename);
        return;
    }
    fread(&header, 512, 1, fNDS);
    int romType = DetectRomType();

    fseek(fNDS, 0x4000, SEEK_SET);
    fread(data, 1, 0x4000, fNDS);

    bool do_decrypt = (endecrypt_option == 'd');
    bool do_encrypt = (endecrypt_option == 'e'); // || (endecrypt_option == 'E');
    unsigned int rounds_offsets = /*(endecrypt_option == 'E') ? 0x2000 :*/ 0x1600;
    unsigned int sbox_offsets = /*(endecrypt_option == 'E') ? 0x2400 :*/ 0x2800;

    // check if ROM is already encrypted
    if (romType == ROMTYPE_NDSDUMPED)
    {
        if (do_decrypt)
        {
            PRINTF("Already decrypted.\n");
        }
        else
        {
            int i;

            if (encrypt_arm9(read32(header.gamecode), data))
            {
                fclose(fNDS);
                return;
            }
            header.secure_area_crc = swiCRC16(0xffff, data, 0x4000);
            header.header_crc = CalcHeaderCRC(header);

            init0(read32(header.gamecode));
            // xor_srand(read32(header.gamecode));

            // clear data after header - will break DSi
            // fseek(fNDS, 0x200, SEEK_SET);
            // for (i=0x200; i<0x1000; i++) fputc(0, fNDS);

            /*			// random data
                        fseek(fNDS, 0x1000, SEEK_SET);
                        for (unsigned int i=0x1000; i<0x4000; i++) fputc(xor_rand()&0xff, fNDS);
            */
            // rounds table
            fseek(fNDS, rounds_offsets, SEEK_SET);
            fwrite(card_hash + 0, 4, 18, fNDS);

            // S-boxes
            for (i = 0; i < 4; i++)
            {
                fseek(fNDS, sbox_offsets + 4 * 256 * i, SEEK_SET);
                fwrite(card_hash + 18 + i * 256, 4, 256, fNDS); // s
            }

            // test patterns
            fseek(fNDS, 0x3000, SEEK_SET);

            for (i = 0x3000; i < 0x3008; i++)
                fputc("\xFF\x00\xFF\x00\xAA\x55\xAA\x55"[i - 0x3000], fNDS);
            for (i = 0x3008; i < 0x3200; i++)
                fputc((u8)i, fNDS);
            for (i = 0x3200; i < 0x3400; i++)
                fputc((u8)(0xFF - i), fNDS);
            for (i = 0x3400; i < 0x3600; i++)
                fputc(0x00, fNDS);
            for (i = 0x3600; i < 0x3800; i++)
                fputc(0xFF, fNDS);
            for (i = 0x3800; i < 0x3A00; i++)
                fputc(0x0F, fNDS);
            for (i = 0x3A00; i < 0x3C00; i++)
                fputc(0xF0, fNDS);
            for (i = 0x3C00; i < 0x3E00; i++)
                fputc(0x55, fNDS);
            for (i = 0x3E00; i < 0x4000 - 1; i++)
                fputc(0xAA, fNDS);
            fputc(0x00, fNDS);

            // write secure 0x800
            fseek(fNDS, 0x4000, SEEK_SET);
            fwrite(data, 1, 0x800, fNDS);

            // calculate CRCs and write header
            fseek(fNDS, 0x4000, SEEK_SET);
            fread(data, 1, 0x4000, fNDS);
            // if (encrypt) encrypt_arm9(*(u32 *)header.gamecode, data);
            header.secure_area_crc = swiCRC16(0xffff, data, 0x4000);

            // header.secure_area_crc = CalcSecureAreaCRC() //false);
            header.logo_crc = CalcLogoCRC(header);
            header.header_crc = CalcHeaderCRC(header);
            fseek(fNDS, 0, SEEK_SET);
            fwrite(&header, 512, 1, fNDS);

            PRINTF("Encrypted.\n");
        }
    }
    else if (romType >= ROMTYPE_ENCRSECURE) // includes ROMTYPE_MASKROM
    {
        if (do_encrypt)
        {
            PRINTF("Already encrypted.\n");
        }
        else
        {
            int i;
            if (decrypt_arm9(read32(header.gamecode), data))
            {
                fclose(fNDS);
                return;
            }

            // clear data after header
            // fseek(fNDS, 0x200, SEEK_SET);
            // for (i=0x200; i<0x4000; i++) fputc(0, fNDS);
            // DSi safe
            fseek(fNDS, 0x1000, SEEK_SET);
            for (i = 0x1000; i < 0x4000; i++)
                fputc(0, fNDS);

            // write secure 0x800
            fseek(fNDS, 0x4000, SEEK_SET);
            fwrite(data, 1, 0x800, fNDS);

            // write header
            fseek(fNDS, 0, SEEK_SET);
            fwrite(&header, 512, 1, fNDS);

            PRINTF("Decrypted.\n");
        }
    }
    else
    {
        PRINTF("File doesn't appear to have a secure area!\n");
        fclose(fNDS);
        return;
    }

    fclose(fNDS);
}

#if defined(FEOS) || !(defined(ARM9) || defined(ARM7))
int xenocrypt(const int argc, const char** argv)
{
    if (argc == 1)
    {
        fprintf(stderr, "XenoCrypt nds...\n");
        return 1;
    }
    int i = 1;
    for (; i < argc; i++)
    {
        fprintf(stderr, "Processing %s... ", argv[i]);
        EnDecryptSecureArea(argv[i], 0);
    }
    return 0;
}
#endif

void EncryptSecureArea(const char* ndsfilename)
{
    EnDecryptSecureArea(ndsfilename, 'e');
}
void DecryptSecureArea(const char* ndsfilename)
{
    EnDecryptSecureArea(ndsfilename, 'd');
}
