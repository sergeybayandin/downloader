#include "request.h"
#include "response.h"

#include "cli/cli_send_request.h"

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>

#include <sys/un.h>
#include <sys/uio.h>
#include <sys/socket.h>

static void set_request_download(struct request *req, const char *url,
	const char *login, const char *password)
{
	struct request_data_download *download = &req->data.download;

	req->type = REQUEST_TYPE_DOWNLOAD;

	strcpy(download->url, url);
	strcpy(download->login, (login != NULL ? login : ""));
	strcpy(download->password, (password != NULL ? password : ""));
}

static void set_daemon_address(struct sockaddr_un *addr)
{
	addr->sun_family = AF_UNIX;
	strcpy(addr->sun_path, DAEMON_SOCKETNAME);
}

int cli_send_request_download(int fd, const char *url,
	const char *login, const char *password)
{
	assert(url != NULL);

	struct request  req;
	struct response resp;

	struct sockaddr_un addr;
	socklen_t          addrlen = sizeof(addr);

	set_request_download(&req, url, login, password);
	set_daemon_address(&addr);

	if (sendto(fd, &req, sizeof(req), 0, (struct sockaddr*)&addr, addrlen) == -1)
		return -1;

	if (recvfrom(fd, &resp, sizeof(resp), 0, (struct sockaddr*)&addr, &addrlen) == -1)
		return -1;

	if (resp.status == RESPONSE_STATUS_ERR) {
		fprintf(stderr, "cli_send_request_download: daemon: %s\n", resp.data.strerror);
		return -1;
	}

	return 0;
}

static void set_request_show_info(struct request *req)
{
	req->type = REQUEST_TYPE_SHOW_INFO;
}

static void print_file_record(const struct file_record *record)
{
	printf("%s\n",  record->url);
	printf("%ld\n", record->size);
	printf("%s\n",  ctime(&record->date));
}

int cli_send_request_show_info(int fd)
{
	struct request  req;
	struct response resp;

	struct sockaddr_un addr;
	socklen_t          addrlen = sizeof(addr);

	int stop = 0;

	set_request_show_info(&req);
	set_daemon_address(&addr);

	if (sendto(fd, &req, sizeof(req), 0, (struct sockaddr*)&addr, addrlen) == -1)
		return -1;

	while (!stop) {
		if (recvfrom(fd, &resp, sizeof(resp), 0, (struct sockaddr*)&addr, &addrlen) == -1) {
			if (errno != EINTR)
				return -1;
			else
				continue;
		}

		switch (resp.status) {
		case RESPONSE_STATUS_ERR  :
			fprintf(stderr, "cli_send_request_show_info: daemon: %s\n", resp.data.strerror);
			return -1;

		case RESPONSE_STATUS_STOP :
			stop = 1;
			break;

		case RESPONSE_STATUS_OK   :
			print_file_record(&resp.data.show_info.record);
			break;
		}
	}

	return 0;
}
