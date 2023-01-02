#include "config.h"

#include "cli/cli_socket.h"

#include <sys/un.h>
#include <sys/socket.h>

int cli_socket(void)
{
	struct sockaddr_un addr;
	socklen_t          addrlen = sizeof(addr);

	int fd;

	if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
		return -1;

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, DAEMON_SOCKETNAME);

	if (connect(fd, (struct sockaddr*)&addr, addrlen) == -1) {
		cli_close(fd);
		return -1;
	}

	return fd;
}

int cli_close(int fd)
{
	return close(fd);
}
