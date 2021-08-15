#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define LEN 64
#define STATUS_LEN 256
#define LOAD_SCALE 1.0d / (1 << SI_LOAD_SHIFT)

char *get_sysinfo(void)
{
    struct sysinfo *info;
    char *ret = malloc(LEN);
    ret[0] = '\0';

    if (sysinfo(info) < 0)
        return ret;

    snprintf(ret, LEN - 1, "M: %.2f GiB L: %.2f %.2f %.2f",
             (double) info->freeram / (1 << 30),
             info->loads[0] * LOAD_SCALE,
             info->loads[1] * LOAD_SCALE,
             info->loads[2] * LOAD_SCALE
        );

    return ret;
}

char *get_time(void)
{
    time_t rawtime;
    struct tm *timeinfo;
    char *ret = malloc(LEN);
    ret[0] = '\0';

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    if (timeinfo == NULL)
        return ret;

    strftime(ret, LEN - 1, "%a %d %b %R", timeinfo);

    return ret;
}

int main(int argc, char *argv[])
{
    Display *dpy = NULL;
    Window win = 0;
    size_t length = 0;
    ssize_t bytes_read = 0;
    char *input = NULL;

    dpy = XOpenDisplay(getenv("DISPLAY"));
    if (dpy == NULL)
    {
        fprintf(stderr, "Can't open display, exiting.\n");
        exit(1);
    }
    win = DefaultRootWindow(dpy);

    char *time;
    char *sys;
    char status[STATUS_LEN];

    while (1)
    {
        time = get_time();
        sys = get_sysinfo();
        snprintf(status, STATUS_LEN - 1, "%s %s", sys, time);

        XStoreName(dpy, win, status);
        XFlush(dpy);

        free(time);
        free(sys);
        sleep(60);
    }

    return 0;
}

// temp (CPU, GPU), VOLUME
