#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define LEN 64
#define STATUS_LEN 256

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
    char status[STATUS_LEN];

    while (1)
    {
        time = get_time();
        snprintf(status, STATUS_LEN - 1, "%s", time);

        XStoreName(dpy, win, status);
        XFlush(dpy);

        free(time);
        sleep(60);
    }

    return 0;
}

// load, temp (CPU, GPU), clock, time, RAM, VOLUME
