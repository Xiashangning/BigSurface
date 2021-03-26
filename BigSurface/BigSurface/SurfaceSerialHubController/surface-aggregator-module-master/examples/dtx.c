#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../module/include/uapi/linux/surface_aggregator/dtx.h"

#define PATH_DTX "/dev/surface/dtx"

int main()
{
	uint8_t buffer[256];
	size_t offs = 0;
	int status;
	int fd;

	fd = open(PATH_DTX, O_RDWR);
	if (fd < 0) {
		printf("error: Could not open file: %s\n", strerror(errno));
		return -1;
	}

	/* Enable events. */
	status = ioctl(fd, SDTX_IOCTL_EVENTS_ENABLE);
	if (status < 0) {
		printf("error: Failed to enable events: %s\n", strerror(errno));
		return -1;
	}

	/* Read events. */
	while (true) {
		struct sdtx_event *event = (struct sdtx_event *)&buffer[0];
		ssize_t n;

		n = read(fd, &buffer[0], sizeof(buffer) - offs);
		if (n < 0) {
			printf("error: Failed to read from file: %s\n",
			       strerror(errno));
			return -1;
		}

		offs += n;

		if (offs >= event->length) {
			printf("info: Received event: code=%u, len=%d\n",
			       event->code, event->length);

			if (event->code == SDTX_EVENT_REQUEST) {
				/* Send confirmation command. */
				status = ioctl(fd, SDTX_IOCTL_LATCH_CONFIRM);
				if (status < 0) {
					printf("error: Failed to send command: %s\n",
					       strerror(errno));
					return -1;
				}
			}

			memmove(buffer, buffer + event->length, offs - event->length);
		}
	}

	return 0;
}
