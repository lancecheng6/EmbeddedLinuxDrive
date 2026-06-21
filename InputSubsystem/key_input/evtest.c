#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/input.h>

int main(int argc, char *argv[])
{
	int fd;
	struct input_event ev;

	if (argc != 2) {
		printf("Usage: %s /dev/input/eventX\n", argv[0]);
		return -1;
	}

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("open %s failed!\n", argv[1]);
		return -1;
	}

	printf("Waiting for input events...\n");
	printf("Press KEY0 on the board!\n\n");

	while (1) {
		read(fd, &ev, sizeof(ev));

		if (ev.type == EV_KEY) {
			printf("EV_KEY: code=%d (%s) value=%d\n",
			       ev.code,
			       ev.code == KEY_0 ? "KEY_0" : "other",
			       ev.value);
		} else if (ev.type == EV_SYN) {
			/* sync event, ignore */
		} else {
			printf("other event: type=%d code=%d value=%d\n",
			       ev.type, ev.code, ev.value);
		}
	}

	close(fd);
	return 0;
}
