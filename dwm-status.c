#include <nvml.h>

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

unsigned int get_GPU_temp(nvmlDevice_t *device)
{
    nvmlReturn_t result;
    unsigned int temp;

    result = nvmlDeviceGetTemperature(*device, NVML_TEMPERATURE_GPU, &temp);
    if (NVML_SUCCESS != result)
        return 0;

    return temp;
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
    dpy = XOpenDisplay(getenv("DISPLAY"));
    if (dpy == NULL)
    {
        fprintf(stderr, "Can't open display, exiting.\n");
        return 1;
    }
    win = DefaultRootWindow(dpy);

    nvmlReturn_t result;
    nvmlDevice_t device;

    result = nvmlInit();
    if(NVML_SUCCESS != result){
        fprintf(stderr, "failed to initialize nvml \n");
        return 1;
    }
    result = nvmlDeviceGetHandleByIndex(0, &device);
    if(NVML_SUCCESS != result){
        fprintf(stderr, "failed to load device \n");
        return 1;
    }

    char *time;
    char *sys;
    unsigned int GPU_temp;
    char status[STATUS_LEN];

    while (1)
    {
        time = get_time();
        sys = get_sysinfo();
        GPU_temp = get_GPU_temp(&device);
        snprintf(status, STATUS_LEN - 1, "%s %s %s",
                 GPU_temp,
                 sys, time
            );

        XStoreName(dpy, win, status);
        XFlush(dpy);

        free(GPU_temp);
        free(time);
        free(sys);
        sleep(60);
    }

    return 0;
}

// temp (CPU, GPU), VOLUME
