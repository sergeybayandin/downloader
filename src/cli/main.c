#include "cli/cli_socket.h"
#include "cli/cli_parse_args.h"
#include "cli/cli_send_request.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	if (cli_parse_args(argc, argv) == -1) {
		if (errno > 0)
			perror("cli_parse_args");
		exit(EXIT_FAILURE);
	}

	int fd, ret[] = {0, 0};

	if ((fd = cli_socket()) == -1) {
		perror("cli_socket");
		exit(EXIT_FAILURE);
	}

	if (pa.url != NULL) {
		ret[0] = cli_send_request_download(fd, pa.url, pa.login, pa.password);
		if (ret[0] == -1 && errno > 0)
			perror("cli_send_request_download");
	}

	if (pa.has_show_info) {
		ret[1] = cli_send_request_show_info(fd);
		if (ret[1] == -1 && errno > 0)
			perror("cli_send_request_show_info");
	}

	if (cli_close(fd) == -1) {
		perror("cli_close");
		exit(EXIT_FAILURE);
	}

	if (ret[0] == -1 || ret[1] == -1)
		exit(EXIT_FAILURE);

	exit(EXIT_SUCCESS);
}
