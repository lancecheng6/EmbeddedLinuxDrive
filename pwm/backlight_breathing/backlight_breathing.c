/*
 * backlight_breathing.c — LCD backlight breathing effect
 *
 * Creates a smooth breathing pattern on the LCD backlight
 * using the PWM-controlled backlight sysfs interface.
 *
 * Compile: arm-linux-gnueabihf-gcc -o backlight_breathing backlight_breathing.c
 * Run:     ./backlight_breathing
 * Stop:    Ctrl+C
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <math.h>

#define BRIGHTNESS_PATH "/sys/class/backlight/backlight/brightness"
#define MAX_BRIGHTNESS  7
#define STEPS           100  /* smoothness of breathing */
#define DELAY_US        30000 /* 30ms per step = ~3s per cycle */

static volatile int keep_running = 1;

void handle_sigint(int sig)
{
    keep_running = 0;
}

int main()
{
    int fd;
    int i, prev = 7;
    char buf[4];

    signal(SIGINT, handle_sigint);

    /* Save current brightness to restore on exit */
    fd = open(BRIGHTNESS_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open brightness");
        return 1;
    }
    int n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n > 0) {
        buf[n] = '\0';
        prev = atoi(buf);
    }

    printf("LCD backlight breathing effect started.\n");
    printf("Press Ctrl+C to stop.\n");
    printf("----------------------------------------\n");

    while (keep_running) {
        for (i = 0; i < STEPS && keep_running; i++) {
            /* Sine wave: 0 → 7 → 0 smoothly */
            double t = (double)i / STEPS * 2 * M_PI;
            int val = (int)((sin(t - M_PI / 2) + 1) / 2 * MAX_BRIGHTNESS + 0.5);
            if (val < 0) val = 0;
            if (val > MAX_BRIGHTNESS) val = MAX_BRIGHTNESS;

            fd = open(BRIGHTNESS_PATH, O_WRONLY);
            if (fd >= 0) {
                snprintf(buf, sizeof(buf), "%d", val);
                write(fd, buf, strlen(buf));
                close(fd);
            }
            usleep(DELAY_US);
        }
    }

    /* Restore original brightness */
    fd = open(BRIGHTNESS_PATH, O_WRONLY);
    if (fd >= 0) {
        snprintf(buf, sizeof(buf), "%d", prev);
        write(fd, buf, strlen(buf));
        close(fd);
    }
    printf("\nBrightness restored to %d\n", prev);
    return 0;
}
