#ifdef NVML_EXISTS
#include <nvml.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <sys/sysinfo.h>
#include <X11/Xlib.h>

#define DATE_LEN 64
#define STATUS_LEN 512
#define LOAD_SCALE 1.0f / (1 << SI_LOAD_SHIFT)

#define REFRESH_RATE 6
#define CPU_TEMP_FILE "/sys/class/thermal/thermal_zone1/temp"

#ifdef BAT_EXISTS
#define BAT_CAP_TEMPLATE "/sys/class/power_supply/BAT%d/capacity"
#define BAT_CAP_LEN 256
char BAT_cap_file[BAT_CAP_LEN];
#endif

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

#ifdef NVML_EXISTS
int get_GPU_temp(nvmlDevice_t *device)
{
    nvmlReturn_t result;
    unsigned int temp;

    result = nvmlDeviceGetTemperature(*device, NVML_TEMPERATURE_GPU, &temp);
    if (NVML_SUCCESS != result)
        return 0;

    return temp;
}
#endif

int read_int_file(char *path)
{
    FILE *fd = fopen(path, "r");
    if (fd == NULL)
        return -1;

    char line[16];
    if (fgets(line, sizeof(line)-1, fd) == NULL)
        return -1;

    fclose(fd);

    return atoi(line);
}

#ifdef BAT_EXISTS
int get_BAT_cap(int index)
{
    snprintf(BAT_cap_file, BAT_CAP_LEN, BAT_CAP_TEMPLATE, index);
    return read_int_file(BAT_cap_file);
}
#endif

int get_CPU_temp()
{
    return read_int_file(CPU_TEMP_FILE) / 1000;
}

void set_time(char *datetime)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    if (timeinfo == NULL)
        return;

    strftime(datetime, DATE_LEN - 1, "%a %d %b %R", timeinfo);
    return;
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

#ifdef NVML_EXISTS
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
#endif

    char *datetime = malloc(DATE_LEN);
    datetime[0] = '\0';
    struct syst_stats *sys = malloc(sizeof(struct syst_stats));
    memset(sys, 0, sizeof(struct syst_stats));
#ifdef NVML_EXISTS
    int GPU_temp;
#endif
    int CPU_temp;
#ifdef BAT_EXISTS
    int BAT_cap;
#endif
    char status[STATUS_LEN];

    int offset = 0;

    while (1)
    {
        offset = 0;
        set_time(datetime);
        set_sysinfo(sys);
        CPU_temp = get_CPU_temp();

#ifdef BAT_EXISTS
        int i = 0;
        while ((BAT_cap = get_BAT_cap(i)) >= 0)
        {
            offset += snprintf(status + offset, STATUS_LEN - offset,
                               "B%d: %d%% ", i, BAT_cap);
            i++;
        }
#endif

#ifdef NVML_EXISTS
        GPU_temp = get_GPU_temp(&device);
        offset += snprintf(status, STATUS_LEN - offset,
                           "TG: %u\u00B0C ", GPU_temp);
#endif

        snprintf(status + offset, STATUS_LEN - offset,
                 "TC: %u\u00B0C M: %.2fGiB L: %.2f %.2f %.2f %s",
                 CPU_temp,
                 sys->free_ram,
                 sys->load1, sys->load5, sys->load15,
                 datetime
            );

        XStoreName(dpy, win, status);
        XFlush(dpy);

        sleep(REFRESH_RATE);
    }
    XCloseDisplay(dpy);
    free(datetime);
    free(sys);
    return 0;
}
