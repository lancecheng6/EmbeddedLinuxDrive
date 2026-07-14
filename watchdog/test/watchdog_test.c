/***************************************************************
 * watchdog_test.c — Watchdog (hardware timer) test program
 *
 * Opens /dev/watchdog, sets a custom timeout, and periodically
 * feeds the dog. If the program is killed (or the system hangs),
 * the watchdog timer expires and the SoC is hard-reset.
 *
 * Usage: ./watchdog_test <timeout_seconds>
 *   e.g. ./watchdog_test 10   # set 10-second timeout
 *
 * Build:  arm-linux-gnueabihf-gcc -o watchdog_test watchdog_test.c
 * Run:    ./watchdog_test 5
 *         (feed the dog for 5 seconds, then stop feeding)
 *         (watchdog expires → board auto-reboots)
 ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <linux/watchdog.h>

#define  WDOG_DEV   "/dev/watchdog"

int main(int argc, char *argv[])
{
    struct watchdog_info info;
    int timeout;
    int time;
    int fd;
    int op;

    if (2 != argc) {
        fprintf(stderr, "usage: %s <timeout>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Open the watchdog device */
    fd = open(WDOG_DEV, O_RDWR);
    if (0 > fd) {
        fprintf(stderr, "open error: %s: %s\n", WDOG_DEV, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Opening starts the timer; stop it first to reconfigure */
    op = WDIOS_DISABLECARD;
    if (0 > ioctl(fd, WDIOC_SETOPTIONS, &op)) {
        fprintf(stderr, "ioctl error: WDIOC_SETOPTIONS: %s\n", strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }

    timeout = atoi(argv[1]);
    if (1 > timeout)
        timeout = 1;

    /* Set the timeout value */
    printf("timeout: %ds\n", timeout);
    if (0 > ioctl(fd, WDIOC_SETTIMEOUT, &timeout)) {
        fprintf(stderr, "ioctl error: WDIOC_SETTIMEOUT: %s\n", strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }

    /* Re-enable the watchdog timer */
    op = WDIOS_ENABLECARD;
    if (0 > ioctl(fd, WDIOC_SETOPTIONS, &op)) {
        fprintf(stderr, "ioctl error: WDIOC_SETOPTIONS: %s\n", strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }

    /* Feed the dog: sleep until just before timeout, then kick */
    time = (timeout * 1000 - 100) * 1000; /* feed 100ms before timeout */
    for ( ; ; ) {

        usleep(time);
        ioctl(fd, WDIOC_KEEPALIVE, NULL);
    }
}
