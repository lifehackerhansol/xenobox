// File++ C implementation
// core-file
// inject-file
// inject-offset (4 bytes)
// inject-filename (len*2 bytes [UCS-2])
// "U-Nai" (5 bytes / Signature)
// inject-filename-length (1 byte [=len])

#include "../xenobox.h"

#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
#include <fcntl.h>
#include <locale.h>
static void initialize()
{
    // setmode(fileno(stdin),O_BINARY);setmode(fileno(stdout),O_BINARY);
    if (!setlocale(LC_ALL, "Japanese"))
    {
        fprintf(stderr, "Fatal: setlocale(LC_ALL,\"Japanese\") failed\n");
        exit(-1);
    }
}
#define ucs2tombs(m, u) wcstombs((char*)(m), (wchar_t*)(u), 768)
#define mbstoucs2(u, m) mbstowcs((wchar_t*)(u), (char*)(m), 256)
#else
#define initialize()
/*
static int wcstombs2(char *dest, const wchar_t *src, size_t n){
  wchar_t *tmp;
  int i=0,ret;
  unsigned char *t,*s=(unsigned char*)src;
  if(sizeof(wchar_t)==2)return wcstombs(dest,src,n);
  tmp=calloc(n,4);
  t=(unsigned char*)tmp;
  for(;i<n/4;i++){
    t[4*i]=s[2*i];
    t[4*i+1]=s[2*i+1];
  }
  ret=wcstombs(dest,tmp,n);
  free(tmp);
  return ret;
}
static int mbstowcs2(wchar_t *dest, const char *src, size_t n){
  wchar_t *tmp;
  int i=0,ret;
  unsigned char *d=(unsigned char*)dest,*t;
  if(sizeof(wchar_t)==2)return mbstowcs(dest,src,n);
  tmp=calloc(n,4);
  t=(unsigned char*)tmp;
  ret=mbstowcs(tmp,src,n);
  for(;i<n;i++){
    d[2*i]=t[4*i];
    d[2*i+1]=t[4*i+1];
  }
  free(tmp);
  return ret;
}
*/
#endif

static int unfilepp2(FILE* f)
{
    FILE* out;
    char name[768];
    u16 wname[256];
    unsigned size, head, tail, headersize;

    size = ftell(f);
    if (fseek(f, size - 6, SEEK_SET))
        return 1;
    if (fread(buf, 1, 6, f) < 6)
        return 1;
    if (memcmp(buf, "U-Nai", 5))
        return 1;
    if (fseek(f, size - (headersize = 10 + buf[5] * 2), SEEK_SET))
        return 1;
    if (fread(buf, 1, 4, f) < 4)
        return 1;
    head = buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24);
    tail = size - head - headersize;
    memset(wname, 0, sizeof(wname));
    if (fread(wname, 2, buf[5], f) < buf[5])
        return 1;
    ucs2tombs((u8*)name, wname);
    name[buf[5]] = 0;
    fseek(f, head, SEEK_SET);

    if (unfilepp2(f))
    { // stack
        fprintf(stderr, "Extracting: $head\n");
        fseek(f, 0, SEEK_SET);
        if (!isatty(fileno(stdout)))
            out = stdout;
        else
            out = fopen("$head", "wb");
        while (head)
        {
            unsigned x = fread(buf, 1, min(BUFLEN, head), f);
            fwrite(buf, 1, x, out);
            head -= x;
        }
        if (out == stdout)
            fflush(stdout);
        else
            fclose(out);
    }
    out = fopen(name, "wb");
    fprintf(stderr, "Extracting: %s\n", name);
    while (tail)
    {
        unsigned x = fread(buf, 1, min(BUFLEN, tail), f);
        fwrite(buf, 1, x, out);
        tail -= x;
    }
    fclose(out);
    fseek(f, headersize, SEEK_CUR);
    return 0;
}

static int unfilepp(const char* file)
{
    FILE* f = fopen(file, "rb");
    int ret;
    if (f == NULL)
        return 1;
    fseek(f, 0, SEEK_END);
    ret = unfilepp2(f);
    fclose(f);
    if (ret)
    {
        fprintf(stderr, "Error: file is corrupted\n");
    }
    return ret;
}

static int __filepp(const char* core, const int argc, const char** argv)
{
    FILE *in1, *in2;
    unsigned char b;
    char n[768];
    u16 wname[256], *w;
    unsigned pos, offset;
    int i = 0;
    if (isatty(fileno(stdout)))
    {
        fprintf(stderr, "Error: output is terminal\n");
        return 1;
    }
    if (core[0] == ':')
        in1 = stdin;
    else if (!(in1 = fopen(core, "rb")))
        return 1;

    while (!feof(in1))
    {
        pos = fread(buf, 1, BUFLEN, in1);
        fwrite(buf, 1, pos, stdout);
    }
    offset = ftell(stdout);
    if (in1 != stdin)
        fclose(in1);

    for (; i < argc; offset = ftell(stdout), i++)
    {
        if (!(in2 = fopen(argv[i], "rb")))
        {
            fprintf(stderr, "Warning: cannot open %s\n", argv[i]);
            continue;
        }
        mbstoucs2(wname, (u8*)argv[i]);
        w = wname;
        for (pos = 0; wname[pos]; pos++)
        {
            if (wname[pos] == '\\' || wname[pos] == '/')
            {
                if (wname[pos + 1])
                    w = wname + pos + 1;
                else
                    wname[pos] = 0;
            }
        }
        ucs2tombs((u8*)n, w);
        fprintf(stderr, "Injecting: %s\n", n);

        while (!feof(in2))
        {
            pos = fread(buf, 1, BUFLEN, in2);
            fwrite(buf, 1, pos, stdout);
        }
        fclose(in2);
        buf[0] = offset & 0xff;
        buf[1] = (offset >> 8) & 0xff;
        buf[2] = (offset >> 16) & 0xff;
        buf[3] = (offset >> 24) & 0xff;
        fwrite(buf, 4, 1, stdout);
        fwrite(w, 2, b = strlen(n), stdout);
        fwrite("U-Nai", 5, 1, stdout);
        fwrite(&b, 1, 1, stdout);
    }
    return 0;
}

int filepp(const int argc, const char** argv)
{
    initialize();
    if (argc < 2)
    {
        fprintf(stderr, "File++ C implementation V.06\n"
                        "Usage:\n"
                        "[extract] file++ File++-file [> output-file]\n"
                        "[inject]  file++ core-file inject-file ... > output-file\n\n");
        return 1;
    }
    if (argc == 2)
        return unfilepp(argv[1]);
    return __filepp(argv[1], argc - 2, argv + 2);
}
