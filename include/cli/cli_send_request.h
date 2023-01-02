#ifndef CLI_SEND_REQUEST_H
#define CLI_SEND_REQUEST_H

int cli_send_request_download(int fd, const char *url,
	const char *login, const char *password);

int cli_send_request_show_info(int fd);

#endif // CLI_SEND_REQUEST_H
