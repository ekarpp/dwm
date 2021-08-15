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

struct syst_stats {
    double free_ram; // in GiB
    double load1;
    double load5;
    double load15;
};

void set_sysinfo(struct syst_stats *sys)
{
    struct sysinfo info;
    if (sysinfo(&info) < 0)
        return;

    sys->free_ram = (double) info.freeram / (1 << 30);
    sys->load1    = info.loads[0] * LOAD_SCALE;
    sys->load5    = info.loads[1] * LOAD_SCALE;
    sys->load15   = info.loads[2] * LOAD_SCALE;
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

unsigned int get_CPU_temp()
{
    const char *path = "/sys/class/thermal/thermal_zone2/temp";
    char line[16];

    FILE *fd = fopen(path, "r");
    if (fd == NULL)
        return 0;

    if (fgets(line, sizeof(line)-1, fd) == NULL)
        return 0;

    return atoi(line) / 1000;
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
    struct syst_stats *sys = malloc(sizeof(struct syst_stats));
    memset(sys, 0, sizeof(struct syst_stats));
    unsigned int GPU_temp;
    unsigned int CPU_temp;
    char status[STATUS_LEN];

    while (1)
    {
        time = get_time();
        set_sysinfo(sys);
        GPU_temp = get_GPU_temp(&device);
        CPU_temp = get_CPU_temp();

        snprintf(status, STATUS_LEN - 1,
                 "TC: %uC TG: %uC M: %.2fGiB L: %.2f %.2f %.2f %s",
                 CPU_temp,
                 GPU_temp,
                 sys->free_ram,
                 sys->load1, sys->load5, sys->load15,
                 time
            );

        XStoreName(dpy, win, status);
        XFlush(dpy);

        free(time);
        sleep(60);
    }
    free(sys);
    return 0;
}

// temp (CPU, GPU), VOLUME