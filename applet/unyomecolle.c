#include "../xenobox.h"

#define Between(n1, x, n2) (((n1) <= (x)) && ((x) <= (n2)))
#define jms1(c)            (Between(0x81, (unsigned char)(c), 0x9f) || Between(0xe0, (unsigned char)(c), 0xfc))
#define jms2(c)            (((unsigned char)(c) != 0x7F) && Between(0x40, (unsigned char)(c), 0xFC))
#define isJMS(p, i)        ((*(p) == 0) ? 0 : (jms1(*(p)) && jms2(*(p + 1)) && (i == 0 || i == 2)) ? 1 : (i == 1) ? 2 : 0)

static int makedir(const char* dir)
{
    char name[512];
    int l;
    int ret = 0, i = 0, ascii = 0; // ret should be -1 if using CreateDirectory

    if (!dir)
        return -1;
    l = strlen(dir);

    memset(name, 0, 512);
    for (; i < l; i++)
    {
        ascii = isJMS(dir + i, ascii);
        if ((dir[i] == '\\' || dir[i] == '/') && !ascii)
        {
            memcpy(name, dir, i + 1);
#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
            ret = mkdir(name); //! CreateDirectory(name,NULL);
#else
            ret = mkdir(name, 0755);
#endif
        }
    }
    return ret;
}

static int recursive(char* sourced, char* sourcef, char* targetd, char* targetf)
{
    DIR* d = opendir(sourced);
    struct dirent* ent;
    if (!d)
        return 1;
    while (ent = readdir(d))
    {
        strcpy(sourcef, ent->d_name);
        strcpy(targetf, ent->d_name);
        if (!strcmp(sourcef, ".") || !strcmp(sourcef, ".."))
            continue;
#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
        if (d->dd_dta.attrib & _A_SUBDIR)
        {
#else
        if (S_ISDIR(sfilemode(sourced)))
        {
#endif
            strcat(sourcef, "/");
            strcat(targetf, "/");
            recursive(sourced, sourced + strlen(sourced), targetd, targetd + strlen(targetd));
        }
        else
        {
            // actual decryption
            int flag = 1;
            if (!strcmp(ent->d_name + strlen(ent->d_name) - 4, ".csv") ||
                !strcmp(ent->d_name + strlen(ent->d_name) - 4, ".xml"))
                flag = 0;
            if (strlen(ent->d_name) >= 6 && ent->d_name[0] == '.' &&
                !strcmp(ent->d_name + strlen(ent->d_name) - 4, ".dat"))
            {
                strncpy(targetf, ent->d_name + 1, strlen(ent->d_name) - 5);
                targetf[strlen(ent->d_name) - 5] = 0;
            }
            printf("%s\n", targetd);
            makedir(targetd);
            FILE *in = fopen(sourced, "rb"), *out = fopen(targetd, "wb");
            int c;
            unsigned char x = 0x80;
            for (; ~(c = fgetc(in)); x++)
            {
                if (x == 0xff)
                    x = 0x80;
                fputc((unsigned char)(c) ^ (flag ? x : 0), out);
            } // just xor. very easy encryption.
            fclose(out);
        }
    }
    closedir(d);
    return 0;
}

int unyomecolle(const int argc, const char** argv)
{
    char *sourced = cbuf + 2048, *targetd = cbuf + 3072;

    if (argc < 3)
    {
        fprintf(stderr, "unyomecolle source-dir/ target-dir/\n");
        return -1;
    }
    strcpy(sourced, argv[1]);
    if (sourced[strlen(sourced) - 1] != '/')
        strcat(sourced, "/");
    strcpy(targetd, argv[2]);
    if (targetd[strlen(targetd) - 1] != '/')
        strcat(targetd, "/");

    recursive(sourced, sourced + strlen(sourced), targetd, targetd + strlen(targetd));
    return 0;
}
