#include "../xenobox.h"
#include "../lib/sha1.h"

static int perform(const char* name)
{
    unsigned char digest[16];
    struct sha1_ctxt sha1ctx;
    sha1_init(&sha1ctx);
    sha1_loop(&sha1ctx, (u8*)name, strlen(name));
    sha1_result(&sha1ctx, digest);

    printf("0x%08X %s\n", read32(digest), name);
    return 0;
}

int nidcalc(const int argc, const char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "nidcalc funcname...\n");
        return 1;
    }
    int i = 1;
    for (; i < argc; i++)
    {
        perform(argv[i]);
    }

    return 0;
}
