/*
 * ap3216cApp.c — AP3216C sensor test program
 *
 * Usage: ./ap3216cApp /dev/ap3216c
 *
 * Reads IR, ALS, and PS data from the AP3216C sensor driver
 * and prints them once per 200ms.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>

int main(int argc, char *argv[])
{
	int fd;
	char *filename;
	unsigned short databuf[3];
	unsigned short ir, als, ps;
	int ret = 0;

	if (argc != 2) {
		printf("Error Usage!\r\n");
		printf("  %s /dev/ap3216c\r\n", argv[0]);
		return -1;
	}

	filename = argv[1];
	fd = open(filename, O_RDWR);
	if (fd < 0) {
		printf("can't open file %s\r\n", filename);
		return -1;
	}

	printf("Reading AP3216C sensor data...\r\n");
	printf("  IR:  infrared (proximity)\r\n");
	printf("  ALS: ambient light\r\n");
	printf("  PS:  proximity (distance)\r\n");
	printf("Press Ctrl+C to exit.\r\n\r\n");
	setbuf(stdout, NULL);	/* disable stdout buffering for real-time output */

	while (1) {
		ret = read(fd, databuf, sizeof(databuf));
		if (ret == sizeof(databuf)) {	/* data read successfully */
			ir  = databuf[0];
			als = databuf[1];
			ps  = databuf[2];
			printf("ir = %4d, als = %5d, ps = %3d\r\n", ir, als, ps);
		}
		usleep(200000); /* 200ms */
	}

	close(fd);
	return 0;
}
