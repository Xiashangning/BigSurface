#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../module/include/uapi/linux/surface_aggregator/cdev.h"

#define PATH_CDEV "/dev/surface/aggregator"

int main()
{
	struct ssam_cdev_request request;
	__u32 version;
	__u16 a, b, c;
	int status;
	int fd;

	fd = open(PATH_CDEV, O_RDWR);
	if (fd < 0) {
		printf("error: Could not open file: %s\n", strerror(errno));
		return -1;
	}

	request.target_category = 0x01;
	request.target_id = 0x01;
	request.command_id = 0x13;
	request.instance_id = 0x00;
	request.flags = 0x01;
	request.status = 0;
	request.payload.data = (__u64)NULL;
	request.payload.length = 0;
	request.response.data = (__u64)&version;
	request.response.length = sizeof(version);

	status = ioctl(fd, SSAM_CDEV_REQUEST, (char *)&request);
	if (status < 0) {
		printf("error: Failed to perform request: %s\n", strerror(errno));
		return -1;
	}

	if (request.status < 0) {
		printf("request error: %d\n", request.status);
		return -1;
	}

	a = (version >> 24) & 0xff;
	b = ((version >> 8) & 0xffff);
	c = version & 0xff;

	printf("firmware version: %u.%u.%u\n", a, b, c);
	return 0;
}
