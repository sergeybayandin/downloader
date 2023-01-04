#include "daemon/daemon_socket.h"
#include "daemon/daemon_handle_requests.h"

#include <stdio.h>
#include <stdlib.h>

#include <curl/curl.h>

int main(void)
{
	int fd;

	if ((fd = daemon_socket()) == -1) {
		perror("daemon_socket");
		exit(EXIT_FAILURE);
	}

	curl_global_init(CURL_GLOBAL_ALL);

	daemon_handle_requests(fd);

	curl_global_cleanup();

	if (daemon_close(fd) == -1) {
		perror("daemon_close");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
