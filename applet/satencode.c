#include "../xenobox.h"

// static char tmp;
// #define swap(a,b) tmp=(a),(a)=(b),(b)=tmp

static char *src, *dst;
static inline void satcode(int encode)
{
    int l = strlen(src), i;
    if (encode)
    {
        // 1: src to dst
        for (i = 0; i < l / 2; i++)
        {
            dst[i * 2] = src[i];
            dst[i * 2 + 1] = src[l - 1 - i];
        }
        if (l & 1)
            dst[i * 2] = src[i];
        // 2: dst to src
        for (i = 0; i < l / 2; i++)
        {
            src[i * 2] = dst[i];
            src[i * 2 + 1] = dst[l - 1 - i];
        }
        if (l & 1)
            src[i * 2] = dst[i];
    }
    else
    {
        int s = l & 1;
        // 1: src to dst
        for (i = 0; i < l / 2; i++)
        {
            dst[i] = src[i * 2];
            dst[i + l / 2 + s] = src[l - 1 - s - i * 2];
        }
        if (l & 1)
            dst[l / 2] = src[l - 1];
        // 2: dst to src
        for (i = 0; i < l / 2; i++)
        {
            src[i] = dst[i * 2];
            src[i + l / 2 + s] = dst[l - 1 - s - i * 2];
        }
        if (l & 1)
            src[l / 2] = dst[l - 1];
    }
}

/*
static int flag;
static char* myfgets_bin(char *buf,int n,FILE *fp){ //accepts LF
    flag=0;
    char *ret=fgets(buf,n,fp);
    if(!ret)return NULL;
    if(strlen(buf)&&buf[strlen(buf)-1]=='\n')flag=1,buf[strlen(buf)-1]=0;
    //if(strlen(buf)&&buf[strlen(buf)-1]=='\r')buf[strlen(buf)-1]=0;
    return ret;
}
*/

static void satcode_file(FILE* in, FILE* out, int encode)
{
    for (; myfgets(src, BUFLEN / 2, in);)
    {
        satcode(encode);
        fwrite(buf, 1, strlen(src), out);
        /*if(flag)*/ fputc('\n', out);
    }
}

int satencode(const int argc, const char** argv)
{
    src = (char*)buf, dst = (char*)buf + BUFLEN / 2;
    if (isatty(fileno(stdin)) || isatty(fileno(stdout)))
    {
        fprintf(stderr, "satencode <in >out\n");
        return 1;
    }
    satcode_file(stdin, stdout, 1);
    return 0;
}

int satdecode(const int argc, const char** argv)
{
    src = (char*)buf, dst = (char*)buf + BUFLEN / 2;
    if (isatty(fileno(stdin)) || isatty(fileno(stdout)))
    {
        fprintf(stderr, "satdecode <in >out\n");
        return 1;
    }
    satcode_file(stdin, stdout, 0);
    return 0;
}
