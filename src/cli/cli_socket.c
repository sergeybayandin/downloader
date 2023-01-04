#include "cli/cli_socket.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>

static int set_unique_address(struct sockaddr_un *addr)
{
	int len;

	len = snprintf(addr->sun_path, sizeof(addr->sun_path), "/tmp/%s", ".downloaderXXXXXX");

	if (mkdtemp(addr->sun_path) == NULL)
		return -1;

	snprintf(addr->sun_path + len, sizeof(addr->sun_path) - len, "/%s", "cli.socket");
	addr->sun_family = AF_UNIX;

	return 0;
}

int cli_socket(void)
{
	struct sockaddr_un addr;
	int                fd;

	if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
		return -1;

	if (set_unique_address(&addr) == -1 || 
	    bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		int err = errno;
		close(fd);
		errno = err;
		return -1;
	}

	return fd;
}

int cli_close(int fd)
{
	struct sockaddr_un addr;
	socklen_t          addrlen = sizeof(addr);

	if (getsockname(fd, (struct sockaddr*)&addr, &addrlen) == -1)
		return -1;

	if (close(fd) == -1 || unlink(addr.sun_path) == -1)
		return -1;

	*strrchr(addr.sun_path, '/') = '\0';

	if (rmdir(addr.sun_path) == -1)
		return -1;

	return 0;
}
