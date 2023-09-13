#include "../xenobox.h"

typedef struct FILEINFO
{
    char name[768];
    struct FILEINFO* next;
} fileinfo;
static fileinfo ftop, *p;

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

static void destroyfilelist()
{
    fileinfo *p = ftop.next, *next;
    if (!ftop.next)
        return;
    while (next = p->next)
    {
        free(p);
        p = next;
    }
    free(p);
    ftop.next = NULL;
}

int dldirename(const int argc, const char** argv)
{
    char* wild = "*.dldi";
    char name[64], *namebodyend, head[0x64];
    struct stat st;
    int i, contentcount = 0;
    FILE* f;

    fprintf(stderr, "dldirename [dir]\n");

    if (argc > 1)
        chdir(argv[1]);
    memset(&ftop, 0, sizeof(fileinfo));
    p = &ftop;
    struct dirent* d;
    DIR* dir = opendir(".");
    for (; d = readdir(dir);)
    {
        if (wildmatch(wild, d->d_name))
        {
            if (stat(d->d_name, &st) || (st.st_mode & S_IFDIR) || st.st_size == 0 || st.st_size > 32 * 1024)
            {
                printf("Err: %s isn't dldi\n", d->d_name);
                continue;
            }
            if (p->next == NULL)
            {
                p->next = (fileinfo*)malloc(sizeof(fileinfo)), memset(p->next, 0, sizeof(fileinfo));
            }
            if (!p->next)
            {
                printf("cannot alloc memory. halt. contentcount==%d\n", contentcount);
                return 1;
            }

            p = p->next;
            strcpy(p->name, d->d_name);
            contentcount++;
        }
    }
    closedir(dir);
    p = &ftop;
    for (i = 0; i < contentcount; i++)
    {
        p = p->next;
        f = fopen(p->name, "rb");
        fread(head, 1, 0x64, f);
        fclose(f);
        if (memcmp(head, "\xed\xa5\x8d\xbf Chishm\0\x1", 13))
        {
            printf("Err: %s signature wrong\n", p->name);
            continue;
        }
        memcpy(name, head + 0x60, 4);
        name[4] = '_';
        strcpy(name + 5, head + 0x10);
        int j = 5;
        for (; j < strlen(name); j++)
            if (!(0x20 <= name[j] && name[j] < 0x7f && !strchr("\\/:*?\"<>|", name[j])))
                name[j] = '_';
        if (!strncmp(p->name, name, strlen(name)))
        { // only compare prefix
            printf("Skip: %s doesn't need to be renamed\n", p->name);
            continue;
        }
        namebodyend = name + strlen(name);
        strcpy(namebodyend, ".dldi");
        if (!stat(name, &st))
        {
            for (j = 0; j < 100; j++)
            {
                sprintf(namebodyend, "%02d.dldi", j);
                if (stat(name, &st))
                    break;
            }
            if (j == 100)
            {
                printf("Err: cannot rename %s\n", p->name);
                continue;
            }
        }
        rename(p->name, name);
        printf("Rename: %s to %s\n", p->name, name);
    }
    destroyfilelist();
    msleep(1500);
    return 0;
}
