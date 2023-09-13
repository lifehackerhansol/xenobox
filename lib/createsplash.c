// #ifdef LIBMSHLSPLASH_CREATE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "libmshlsplash.h"
#include "libnsbmp.h"
#include "minIni.h"

#include <dirent.h>
#include <unistd.h>

#define IMGMAX (256 * 192 * 4 + 0x200)
#define B15MAX (256 * 192 * 2 + 0x200)
// static u8 imgbuf[IMGMAX];
// static u16 decompbuf[B15MAX],compbuf[IMGMAX]; //compbuf is used also as temporary buffer
// static u16 first[B15MAX];
static u8* imgbuf;
static u16 *decompbuf, *compbuf;
static u16* first;

extern u8 buf[];
u8* iSakuReal_palette = buf; //[32768];

static bool wildmatch(const char* pattern, const char* compare)
{
    switch (*pattern)
    {
        case '?': // 0x3f
            return wildmatch(pattern + 1, compare + 1);
        case '*': // 0x2a
            return wildmatch(pattern + 1, compare) || (*compare && wildmatch(pattern, compare + 1));
        default:
            if (!*pattern && !*compare)
                return true;
            if (*pattern != *compare)
                return false;
            return wildmatch(pattern + 1, compare + 1);
    }
}

static const int DitherTable4Data8[16] = { 0, 4, 1, 5, 6, 2, 7, 3, 1, 5, 0, 4, 7, 3, 6, 2 };

static unsigned char DitherTable4(int c, int x, int y)
{
    int sx = x & 3, sy = y & 3;
    c += DitherTable4Data8[sx * 4 + sy];
    if (c > 0xff)
        c = 0xff;
    return c & ~7;
}

static void* bitmap_create(int width, int height, unsigned int state)
{
    return compbuf;
}

static void bitmap_set_suspendable(void* bitmap, void* private_word,
                                   void (*invalidate)(void* bitmap, void* private_word))
{
}

// static void invalidate(void *bitmap, void *private_word){}

static unsigned char* bitmap_get_buffer(void* bitmap)
{
    return bitmap;
}

static size_t bitmap_get_bpp(void* bitmap)
{
    return 4;
}

static void bitmap_destroy(void* bitmap)
{
} // using local buffer so doesn't destroy

static int imagedecode(char* name, int* height, int dither)
{
    struct stat st;
    int x, y;
    FILE* in = fopen(name, "rb");
    if (!in)
        return -1;
    fstat(fileno(in), &st);
    if (!strcasecmp(name + strlen(name) - 4, ".bmp"))
    {
        fread(imgbuf, 1, st.st_size, in);
        fclose(in);
        bmp_image gif;
        bmp_bitmap_callback_vt vt = { bitmap_create, bitmap_destroy, bitmap_set_suspendable, bitmap_get_buffer,
                                      bitmap_get_bpp };

        bmp_create(&gif, &vt);
        if (bmp_analyse(&gif, st.st_size, imgbuf) || gif.width != 256 || gif.height == 0 || gif.height > 192 ||
            bmp_decode(&gif))
        {
            bmp_finalise(&gif);
            return 1;
        }
        *height = gif.height;
        bmp_finalise(&gif);
        for (y = 0; y < *height; y++)
            for (x = 0; x < 256; x++)
            {
                u32 coor = y * 256 + x;
                u8 b = ((((u32*)compbuf)[coor] & 0xff0000) >> 16) & 0xff;
                u8 g = ((((u32*)compbuf)[coor] & 0x00ff00) >> 8) & 0xff;
                u8 r = ((((u32*)compbuf)[coor] & 0x0000ff) >> 0) & 0xff;
                if (dither)
                {
                    r = DitherTable4(r, x, y);
                    g = DitherTable4(g, x, y);
                    b = DitherTable4(b, x, y);
                }
                decompbuf[coor] = (1 << 15) | ((b >> 3) << 10) | ((g >> 3) << 5) | ((r >> 3) << 0);
            }
    }
    return 0;
}

static int _createsplash(const char* path, int version)
{
    char wild[768], sdir[768], cwd[768];
    u8 buf[2];
    u32 flags, nframe = 0;
    int fps, waitforterminate, alreadyalldraw, dither;
    FILE* out;
    int i, x, y, height = 0;
    struct stat st;

    if (version != SPLASH_MOONSHELL1 && version != SPLASH_MOONSHELL2 && version != SPLASH_ISAKUREAL &&
        version != SPLASH_M3SAKURA)
        return 1;
    ini_gets("ConvertSetting", "SourceFileMask", "", wild, 768, path);
    if (strlen(wild) < 4 ||
        (strcasecmp(wild + strlen(wild) - 4,
                    ".bmp") /*&&strcasecmp(wild+strlen(wild)-4,".ppm")&&strcasecmp(wild+strlen(wild)-4,".png")*/))
        return 2;
    fps = ini_getl("ConvertSetting", "SourceFPS", 0, path);
    if (fps <= 0)
        return 2;
    waitforterminate = ini_getl("ConvertSetting", "WaitForTerminate", 0, path);
    alreadyalldraw = ini_getl("ConvertSetting", "AlreadyAllDraw", 0, path);
    dither = ini_getl("ConvertSetting", "UseDither24to15bit", 0, path);
    flags = (waitforterminate ? 1 : 0) | (alreadyalldraw ? 2 : 0);

    fprintf(stderr, "%s splash.\n",
            version == SPLASH_MOONSHELL1   ? "MoonShell1 (gbalzss)"
            : version == SPLASH_MOONSHELL2 ? "MoonShell2 (zlib + diff encode)"
            : version == SPLASH_ISAKUREAL  ? "iSakuReal (lzss varient)"
            : version == SPLASH_M3SAKURA   ? "M3Sakura (raw)"
                                           : ""); // unreachable
    if (version == SPLASH_ISAKUREAL && dither)
    {
        fprintf(stderr, "As iSakuReal splash uses palette, dithering disabled.\n");
        dither = 0;
    }
    if (version == SPLASH_M3SAKURA)
    {
        fprintf(stderr, "M3Sakura; flags/fps are discard.\n");
        flags = 0x496e614d;
    }

    // OK, configured. Let's proceed to making splash.
    strcpy(sdir, path);
    getcwd(cwd, 768);
    {
        i = strlen(sdir);
        for (; i > 0; i--)
            if (sdir[i - 1] == '\\' || sdir[i - 1] == '/')
            {
                sdir[i] = 0;
                chdir(sdir);
                break;
            }
    }

    struct dirent* d;
    DIR* dir = opendir("."); // opendir("xxx/") isn't valid.
    if (!dir)
    {
        chdir(cwd);
        return 2;
    }
    for (; d = readdir(dir);)
        if (wildmatch(wild, d->d_name))
        {
            if (stat(d->d_name, &st) || st.st_size == 0 || st.st_size > IMGMAX)
                continue;
            nframe++;
        }
    rewinddir(dir);
    u32* array = NULL;
    splash_frame* head = NULL;
    if (version == SPLASH_M3SAKURA)
    {
        if (nframe > 126)
        {
            closedir(dir);
            chdir(cwd);
            return 3;
        }
        array = (u32*)malloc(4 * nframe);
        if (!array)
        {
            closedir(dir);
            chdir(cwd);
            return 2;
        }
    }
    else
    {
        head = (splash_frame*)malloc(16 * nframe);
        if (!head)
        {
            closedir(dir);
            chdir(cwd);
            return 2;
        }
    }
    out = fopen("splash.ani", "wb");
    if (!out)
    {
        free(head);
        free(array);
        closedir(dir);
        chdir(cwd);
        return 2;
    }
    fwrite(&flags, 1, 4, out);
    fwrite(&nframe, 1, 4, out);
    if (version == SPLASH_M3SAKURA)
    {
        fwrite(array, 4, nframe, out);
        fseek(out, 0x200, SEEK_SET);
    }
    else
    {
        fwrite(head, 16, nframe, out);
    }
    for (i = 0; d = readdir(dir);)
    {
        if (wildmatch(wild, d->d_name))
        {
            fprintf(stderr, "Encoding %03d / %03d\r", i, nframe - 1);
            if (stat(d->d_name, &st) || st.st_size == 0 || st.st_size > IMGMAX)
                continue;
            if (version == SPLASH_M3SAKURA)
            {
                array[i] = ftell(out);
            }
            else
            {
                head[i].vsync = i * 60 / fps;
                head[i].offset = ftell(out);
            }
            if (imagedecode(d->d_name, &height, dither))
            {
                fclose(out);
                free(head);
                free(array);
                closedir(dir);
                chdir(cwd);
                return 20;
            }
            if (version == SPLASH_MOONSHELL1)
            {
                memstream mdecomp, mcomp;
                mopen(decompbuf, 256 * height * 2, &mdecomp);
                mopen(compbuf, B15MAX, &mcomp);
                gbalzssEncode(&mdecomp, &mcomp, false); // MoonShell1.x uses swiDecompressLZ77Wram, so false is allowed.
                head[i].size1 = mtell(&mcomp);
                head[i].size2 = 256 * height * 2;
                fwrite(compbuf, 1, mtell(&mcomp), out);
            }
            if (version == SPLASH_ISAKUREAL)
            {
                u32 flags0 = 0, flags1 = ((256 * height * 2) << 8) | 0xf0;
                u32 offset = ftell(out);
                fwrite(&nframe, 1, 4, out); // dummy
                memset(iSakuReal_palette, 0xff, 0x8000);
                int palcnt = 0, coor = 0;
                for (; coor < 256 * height; coor++)
                { // make palette
                    if (iSakuReal_palette[decompbuf[coor] & 0x7fff] == 0xff)
                    {
                        if (palcnt == 0xff)
                        {
                            fprintf(stderr, "palette limit exceeded.\n");
                            exit(1);
                        }
                        iSakuReal_palette[decompbuf[coor] & 0x7fff] = palcnt++;
                        fwrite(&decompbuf[coor], 1, 2, out);
                    }
                }
                if ((palcnt & 1) == 1)
                { // need alignment
                    palcnt++;
                    fwrite(&flags0, 1, 2, out);
                }
                u32 offset2 = ftell(out);
                fseek(out, offset, SEEK_SET);
                fwrite(&palcnt, 1, 4, out);
                fseek(out, offset2, SEEK_SET);
                fwrite(&flags1, 1, 4, out);

                for (coor = 0; coor < 256 * height; coor++)
                { // only copy...
                    if ((coor & 0x3f) == 0)
                        fwrite(&flags0, 1, 4, out);
                    fputc(iSakuReal_palette[decompbuf[coor] & 0x7fff], out);
                }
                head[i].size1 = ftell(out) - offset;
                head[i].size2 = 256 * height * 2;
                // while(ftell(out)&3)fputc(0,out); //need alignment
            }
            if (version == SPLASH_M3SAKURA)
            {
                const u16 const0 = 0;
                int coor = 256 * height;
                fwrite(decompbuf, 2, coor, out);
                for (; coor < 256 * 192; coor++)
                {
                    fwrite(&const0, 2, 1, out); // fputc(0,out);fputc(0,out); //fill with trans
                }
            }
            if (version == SPLASH_MOONSHELL2)
            {
                if (i == 0)
                {
                    memcpy(first, decompbuf, 256 * height * 2);
                    int compsize = zlib_compress(decompbuf, 256 * height * 2, compbuf, B15MAX);
                    head[i].size1 = 256 * height;
                    head[i].size2 = compsize;
                    fwrite(compbuf, 1, compsize, out);
                }
                else
                {
                    // diff
                    int nullcnt = 0;
                    for (y = 0; y < height; y++)
                        for (x = 0; x < 256; x++)
                        {
                            u32 coor = y * 256 + x;
                            if (first[coor] != decompbuf[coor])
                            {
                                if (nullcnt)
                                {
                                    if (nullcnt > 0x7fff)
                                    { /// ***
                                        write16(buf, 0x7fff);
                                        fwrite(buf, 1, 2, out);
                                        nullcnt -= 0x7fff;
                                    }
                                    write16(buf, nullcnt);
                                    fwrite(buf, 1, 2, out);
                                    nullcnt = 0;
                                }
                                write16(buf, decompbuf[coor]);
                                fwrite(buf, 1, 2, out);
                            }
                            else
                            {
                                nullcnt++;
                            }
                        }
#if 0
					if(nullcnt){
						if(nullcnt>0x7fff){
							write16(buf,0x7fff);
							fwrite(buf,1,2,out);
							nullcnt-=0x7fff;
						}
						write16(buf,nullcnt);
						fwrite(buf,1,2,out);
					}
#endif
                    head[i].size1 = 256 * height;
                    head[i].size2 = (ftell(out) - head[i].offset) / 2;
                }
            }
            i++;
            if (i == nframe)
                break;
        }
    }
    fseek(out, 8, SEEK_SET);
    if (version == SPLASH_M3SAKURA)
    {
        fwrite(array, 4, nframe, out);
    }
    else
    {
        fwrite(head, 16, nframe, out);
    }
    fclose(out);
    free(head);
    free(array);
    closedir(dir);
    chdir(cwd);
    fprintf(stderr, "Encode done.          \n");
    return i == nframe ? 0 : 10;
}

int createsplash(const char* path, int version)
{
    imgbuf = (u8*)malloc(IMGMAX);
    decompbuf = (u16*)malloc(B15MAX);
    compbuf = (u16*)malloc(IMGMAX);
    first = (u16*)malloc(B15MAX);
    if (!imgbuf || !decompbuf || !compbuf || !first)
    {
        fprintf(stderr, "not enough memory.\n");
        free(imgbuf);
        free(decompbuf);
        free(compbuf);
        free(first);
        return -1;
    }
    int ret = _createsplash(path, version);
    free(imgbuf);
    free(decompbuf);
    free(compbuf);
    free(first);
    return ret;
}

// #endif
