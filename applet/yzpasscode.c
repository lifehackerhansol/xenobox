#include "../xenobox.h"

static unsigned char passcode[5];
int getpasscode(const char* pass)
{
    const int len = strlen(pass);
    unsigned char input_passwd_ucode = len;
    unsigned int a, b, c, d;
    a = b = c = d = (unsigned int)input_passwd_ucode;

    int i = 0;
    for (; i < len; i++)
    {
        input_passwd_ucode += (unsigned char)(pass[i]) ^ i;
        unsigned int e = (unsigned int)(pass[i]) ^ i;
        a += e;
        b -= e;
        c ^= e;
        d *= e;
    }
    a *= b;
    a += d;
    a += c;

    passcode[0] = (unsigned char)((a >> 24) & 0xFF);
    passcode[1] = (unsigned char)((a >> 16) & 0xFF);
    passcode[2] = (unsigned char)((a >> 8) & 0xFF);
    passcode[3] = (unsigned char)((a)&0xFF);
    passcode[4] = input_passwd_ucode;

    printf("pub-%02x%02x-%02x%02x-%02x\n", passcode[0], passcode[1], passcode[2], passcode[3], passcode[4]);
    return 0;
}

int yzpasscode(const int argc, const char** argv)
{
    int i = 1;
    for (; i < argc; i++)
    {
        getpasscode(argv[i]);
    }
    return 0;
}
