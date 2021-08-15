#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

int main(int argc, char * argv[])
{
        Display * dpy = NULL;
        Window win = 0;
        size_t length = 0;
        ssize_t bytes_read = 0;
        char * input = NULL;

        dpy = XOpenDisplay(getenv("DISPLAY"));
        if (dpy == NULL)
        {
                fprintf(stderr, "Can't open display, exiting.\n");
                exit(1);
        }
        win = DefaultRootWindow(dpy);

        time_t rawtime;
        struct tm * timeinfo;
        char text[128];
        while (1)
        {
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            strftime(text, 500, "%a %d %b %R", timeinfo);
            XStoreName(dpy, win, text);
            XFlush(dpy);
            sleep(60);
        }

        return 0;
}

// load, temp (CPU, GPU), clock, time, RAM, VOLUME
