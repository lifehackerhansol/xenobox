#include "../xenobox.h"

int getjavacert(const int argc, const char** argv)
{
    if (isatty(fileno(stdin)))
    {
        fprintf(stderr, "unzip -p FILE.apk/jar \"META-INF/*.?SA\" | getjavacert\n"
                        "7z x -so FILE.apk/jar \"META-INF/*.?SA\" 2>/dev/null | getjavacert\n");
        return -1;
    }
    fread(buf, 1, 0x38, stdin); // fseek(stdin,0x38,SEEK_SET);
    fread(buf + (BUFLEN / 2), 1, 2, stdin);
    if (buf[(BUFLEN / 2)] != 0x30 || buf[(BUFLEN / 2) + 1] > 0x84)
    {
        fprintf(stderr, "cannot parse header\n");
        return 1;
    }
    u32 len = 0;
    if (buf[(BUFLEN / 2) + 1] < 0x80)
        len = buf[(BUFLEN / 2) + 1];
    u32 i = 0, j = 0;
    for (; i < buf[(BUFLEN / 2) + 1] - 0x80; i++)
    {
        buf[(BUFLEN / 2) + 2 + i] = fgetc(stdin);
        len = (len << 8) + buf[(BUFLEN / 2) + 2 + i];
    }
    if (len > BUFLEN / 4)
    {
        fprintf(stderr, "length too large (%d)\n", len);
        return 1;
    }
    fread(buf + (BUFLEN / 2) + 2 + i, 1, len, stdin); // total=2+i+len
    for (; j < 2 + i + len; j++)
    {
        int x = buf[(BUFLEN / 2) + j] >> 4, y = buf[(BUFLEN / 2) + j] & 0xf;
        buf[2 * j + 0] = x > 9 ? (x - 10 + 'a') : (x + '0');
        buf[2 * j + 1] = y > 9 ? (y - 10 + 'a') : (y + '0');
    }
    buf[2 * j + 0] = 0;
    printf("%s", cbuf);
    if (isatty(fileno(stdout)))
        puts("");

    return 0;
}

#if 0
int asn1derdump(const int argc, const char **argv){
	fprintf(stderr,"not implemented yet.\n");return -1;
}
#endif
