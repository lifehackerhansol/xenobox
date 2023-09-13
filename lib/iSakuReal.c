#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libmshlsplash.h"

/*
    offset(*): abcdefg (7bit)
    a:   use 1pixelcopy or not
    b-e: length(4bit) 0-15 +3pixels
    f/g: mode(2bit)
            r16w16/r16w32/r32w16/r32w32

    I thank so much to cheryl and mbmax, who gave me the M3Sakura 1.12 source code.
    Actually the fastlzss16decpalasm_body() was written is asm, but I have rewritten it in C.
*/

int iSakuRealDecode(memstream* comp, memstream* decomp, memstream* decompbuf, u32 palcnt, u16* pal)
{
    u32 flags = 0, flagscnt = 1;
    u32 x = mread32(comp);
    if ((x & 0xff) != 0xf0)
    {
        printf("signature error\n");
        return 1;
    }
    while (1)
    {
        if (!mavail(comp))
            break;
        flagscnt--;
        if (flagscnt == 0)
        {
            flagscnt = 32;
            flags = mread32(comp);
        }
        if (!(flags & 0x80000000))
        {
            // copyone
            flags <<= 1;
            u16 col = read16(pal + mgetc(comp));
            mwrite16(col, decomp);
            col = read16(pal + mgetc(comp));
            mwrite16(col, decomp);
        }
        else
        {
            // copyblock
            flags <<= 1;
            u16 flag = mread16(comp);
            u32 offset = flag & 0xf000;
            u32 copysrc = flag & 0x0fff;
            if (copysrc == 0)
                offset |= 0x10000;
            mseek(decompbuf, mtell(decomp) - ((copysrc + 1) << 1), SEEK_SET);
            // printf("%d\n",mtell(decompbuf));

            // just ignore because we always use 16bit IO
            // if(!(mtell(decomp)&3))offset|=0x0400;
            // if(!(mtell(decompbuf)&3))offset|=0x0800;

            offset >>= 10; // ...(*)
            if (offset > 0x7f)
            {
                printf("data error offset=%d\n", offset);
                return 1;
            }
            u32 onepixel = (offset & 0x40) >> 6;
            u32 length = (offset & 0x3c) >> 2;
            // u32 mode=(offset&0x3);
            // printf("%04x %d %d %d\n",copysrc,onepixel,length,mode);

            int idx = 0;
            if (onepixel)
            {
                u16 pixel = mread16(decompbuf);
                for (; idx < (length + 3); idx++)
                    mwrite16(pixel, decomp);
            }
            else
            {
                for (; idx < (length + 3); idx++)
                    mwrite16(mread16(decompbuf), decomp);
            }
        }
    }
    return 0; // everything is ok
}
