#include "cli/cli_socket.h"

#include <sys/socket.h>

int cli_socket(void)
{
	return socket(AF_UNIX, SOCK_DGRAM, 0);
}

int cli_close(int fd)
{
	return close(fd);
}
