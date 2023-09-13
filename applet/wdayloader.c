#include "../xenobox.h"
#include <time.h>

static char* wdays[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
int wdayloader(const int argc, const char** argv)
{
    time_t timer;
    struct tm* pt;
    time(&timer);
    pt = localtime(&timer);
    int wday = pt->tm_wday;

    for (;; msleep(200))
    {
        time(&timer);
        pt = localtime(&timer);
        int t = pt->tm_hour * 3600 + pt->tm_min * 60 + pt->tm_sec;
        if (t < 2)
            break;
        // int n=t*25/86400,i=0;
        // for(;i<n;i++)progress[i]='*';
        fprintf(stderr, "Loading %s.exe %02d%%... Remaining %02d:%02d:%02d\r", wdays[(wday + 1) % 7], t * 100 / 86400,
                (86400 - t) / 3600, (86400 - t) / 60 % 60, (86400 - t) % 60);
    }

    // int n=25,i=0;
    // for(;i<n;i++)progress[i]='*';
    fprintf(stderr, "Loaded %s.exe 100%%! Remaining %02d:%02d:%02d    \n", wdays[(wday + 1) % 7], 0, 0, 0);
    return 0;
}
