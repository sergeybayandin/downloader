#include "config.h"

#include "daemon/daemon_socket.h"

#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>

int daemon_socket(void)
{
	struct sockaddr_un addr;
	socklen_t          addrlen = sizeof(addr);

	int fd;

	if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
		return -1;

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, DAEMON_SOCKETNAME);

	if (bind(fd, (struct sockaddr*)&addr, addrlen) == -1) {
		close(fd);
		return -1;
	}

	return fd;
}

int daemon_close(int fd)
{
	if (close(fd) == -1 || unlink(DAEMON_SOCKETNAME) == -1)
		return -1;
	return 0;
}
