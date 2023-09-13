#include "../xenobox.h"

int dlsymtest(const int argc, const char** argv)
{
#ifdef NODLOPEN
    fprintf(stderr, "dlopen() disabled\n");
    return -1;
#else

    if (argc < 2)
    {
        fprintf(stderr, "dlsymtest soname func...\n");
        return 1;
    }
#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
    HMODULE h;
#else
    void* h;
#endif
    fprintf(stderr, "dlopen(%s) ", argv[1]);
    h = LoadLibraryA(argv[1]);
    if (!h)
    {
        fprintf(stderr, "failed\n");
        return 2;
    }
    fprintf(stderr, "ok\n");
    int i = 2;
    for (; i < argc; i++)
    {
        // fprintf(stderr,"dlsym(%s,%s) ",argv[1],argv[i]);
        fprintf(stderr, "dlsym(%s) ", argv[i]);
        fprintf(stderr, GetProcAddress(h, argv[i]) ? "ok\n" : "failed\n");
    }
    FreeLibrary(h);
    return 0;
#endif
}
