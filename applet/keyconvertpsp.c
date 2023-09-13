#include "../xenobox.h"
enum PspCtrlButtons
{
    PSP_CTRL_SELECT = 0x000001,
    PSP_CTRL_START = 0x000008,
    PSP_CTRL_UP = 0x000010,
    PSP_CTRL_RIGHT = 0x000020,
    PSP_CTRL_DOWN = 0x000040,
    PSP_CTRL_LEFT = 0x000080,
    PSP_CTRL_LTRIGGER = 0x000100,
    PSP_CTRL_RTRIGGER = 0x000200,
    PSP_CTRL_TRIANGLE = 0x001000,
    PSP_CTRL_CIRCLE = 0x002000,
    PSP_CTRL_CROSS = 0x004000,
    PSP_CTRL_SQUARE = 0x008000,
    PSP_CTRL_HOME = 0x010000,
    PSP_CTRL_HOLD = 0x020000,
    PSP_CTRL_NOTE = 0x800000,
    PSP_CTRL_SCREEN = 0x400000,
    PSP_CTRL_VOLUP = 0x100000,
    PSP_CTRL_VOLDOWN = 0x200000,
    PSP_CTRL_WLAN_UP = 0x040000,
    PSP_CTRL_REMOTE = 0x080000,
    PSP_CTRL_DISC = 0x1000000,
    PSP_CTRL_MS = 0x2000000,
    PSP_CTRL_SLIDE = 0x20000000,
};

int keyconvertpsp(const int argc, const char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "keyconvertpsp [int|keys]\n");
        return 1;
    }
    unsigned int keys = strtoul(argv[1], NULL, 0);
    int i = 0;
    if (keys)
    { // int to keys
#define INTTOKEY(KEY, KEY_PRINT) \
    if (keys & PSP_CTRL_##KEY)   \
    {                            \
        if (i)                   \
            putchar('+');        \
        i++;                     \
        printf(#KEY_PRINT);      \
    }
        INTTOKEY(SELECT, SELECT)
        INTTOKEY(START, START)
        INTTOKEY(UP, UP)
        INTTOKEY(RIGHT, RIGHT)
        INTTOKEY(DOWN, DOWN)
        INTTOKEY(LEFT, LEFT)
        INTTOKEY(LTRIGGER, LTRIGGER)
        INTTOKEY(RTRIGGER, RTRIGGER)
        INTTOKEY(TRIANGLE, TRIANGLE)
        INTTOKEY(CIRCLE, CIRCLE)
        INTTOKEY(CROSS, CROSS)
        INTTOKEY(SQUARE, SQUARE)
        INTTOKEY(SELECT, SELECT)
        INTTOKEY(HOME, HOME)
        INTTOKEY(HOLD, HOLD)
        INTTOKEY(NOTE, NOTE)
        INTTOKEY(SCREEN, SCREEN)
        INTTOKEY(VOLUP, VOLUP)
        INTTOKEY(VOLDOWN, VOLDOWN)
        INTTOKEY(WLAN_UP, WLAN)
        INTTOKEY(REMOTE, REMOTE)
        INTTOKEY(DISC, DISC)
        INTTOKEY(MS, MS)
        INTTOKEY(SLIDE, SLIDE)
#undef INTTOKEY
        putchar('\n');
    }
    else
    { // keys to val
        char* buttons = (char*)argv[1];
        for (; *buttons;)
        {
            for (i = 0; buttons[i]; i++)
            {
                if (!buttons[i] || buttons[i] == ',' || buttons[i] == '+' || buttons[i] == '|')
                    break;
            }
#define KEYTOINT(KEY_PRINT, KEY)                                         \
    if (strlen(#KEY_PRINT) == i && !strncasecmp(buttons, #KEY_PRINT, i)) \
    {                                                                    \
        keys |= PSP_CTRL_##KEY;                                          \
    }
            KEYTOINT(SELECT, SELECT)
            KEYTOINT(START, START)
            KEYTOINT(UP, UP)
            KEYTOINT(RIGHT, RIGHT)
            KEYTOINT(DOWN, DOWN)
            KEYTOINT(LEFT, LEFT)
            KEYTOINT(LTRIGGER, LTRIGGER)
            KEYTOINT(RTRIGGER, RTRIGGER)
            KEYTOINT(L, LTRIGGER)
            KEYTOINT(R, RTRIGGER)
            KEYTOINT(TRIANGLE, TRIANGLE)
            KEYTOINT(CIRCLE, CIRCLE)
            KEYTOINT(CROSS, CROSS)
            KEYTOINT(SQUARE, SQUARE)
            KEYTOINT(SELECT, SELECT)
            KEYTOINT(HOME, HOME)
            KEYTOINT(HOLD, HOLD)
            KEYTOINT(NOTE, NOTE)
            KEYTOINT(SCREEN, SCREEN)
            KEYTOINT(VOLUP, VOLUP)
            KEYTOINT(VOLDOWN, VOLDOWN)
            KEYTOINT(WLAN, WLAN_UP)
            KEYTOINT(REMOTE, REMOTE)
            KEYTOINT(DISC, DISC)
            KEYTOINT(MS, MS)
            KEYTOINT(SLIDE, SLIDE)
#undef KEYTOINT
            buttons += i + 1;
        }
        printf("0x%08x\n", keys);
    }
    return 0;
}
