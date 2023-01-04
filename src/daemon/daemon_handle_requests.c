#include "request.h"
#include "response.h"

#include "daemon/daemon_handle_requests.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <curl/curl.h>

#include <unistd.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <linux/limits.h>

struct download_routine_arg {
	struct request_data_download download;
	int                          fd;
};

struct show_info_routine_arg {
	struct request_data_show_info show_info;
	int                           fd;
};

static int create_connected_socket(struct sockaddr_un *addr)
{
	int fd;

	if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
		return -1;

	if (connect(fd, (struct sockaddr*)addr, sizeof(*addr)) == -1) {
		int save = errno;
		close(fd);
		errno = save;
		return -1;
	}

	return fd;
}

static void sendto_error_response(int fd, struct sockaddr_un *addr, const char *strerror)
{
	struct response resp;

	resp.status = RESPONSE_STATUS_ERR;
	strncpy(resp.data.strerror, strerror, STRERROR_MAXLEN);

	sendto(fd, &resp, sizeof(resp), 0, (struct sockaddr*)addr, sizeof(*addr));
}

static void send_error_response(int fd, const char *strerror)
{
	struct response resp;

	resp.status = RESPONSE_STATUS_ERR;
	strncpy(resp.data.strerror, strerror, STRERROR_MAXLEN);

	send(fd, &resp, sizeof(resp), 0);
}

static int create_detached_thread(void *(*routine)(void*), void *arg)
{
	pthread_attr_t attr;

	pthread_t tid;
	int       status;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	status = pthread_create(&tid, &attr, routine, arg);

	pthread_attr_destroy(&attr);

	if (status != 0) {
		errno = status;
		return -1;
	}

	return 0;
}

static int download_file(CURL *handle, const char *path,
	const struct request_data_download *download, CURLcode *code)
{
	FILE *file;

	if ((file = fopen(path, "w")) == NULL)
		return -1;

	curl_easy_setopt(handle, CURLOPT_WRITEDATA, file);
	curl_easy_setopt(handle, CURLOPT_URL, download->url);
	curl_easy_setopt(handle, CURLOPT_DISALLOW_USERNAME_IN_URL, 1);

	if (download->login[0] != '\0')
		curl_easy_setopt(handle, CURLOPT_USERNAME, download->login);

	if (download->password[0] != '\0')
		curl_easy_setopt(handle, CURLOPT_PASSWORD, download->password);

	*code = curl_easy_perform(handle);
		
	fclose(file);

	if (*code != CURLE_OK)
		return -1;

	return 0;
}

static int create_downloads_directory(const char *path)
{
	mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

	if (mkdir(path, mode) == -1 && errno != EEXIST)
		return -1;

	return 0;
}

static void *download_routine(struct download_routine_arg *arg)
{
	CURL     *handle;
	CURLcode code = CURLE_OK;

	struct response resp;

	int  pathlen;
	char path[PATH_MAX], *filename;
	
	pathlen = snprintf(path, sizeof(path), "%s/%s",
		getenv("HOME"), "Downloads");

	if (create_downloads_directory(path) == -1) {
		send_error_response(arg->fd, strerror(errno));
		goto exit;
	}

	filename = strrchr(arg->download.url, '/');
	pathlen += snprintf(path + pathlen, sizeof(path) - pathlen, "/%s",
		(filename == NULL || filename[1] == '\0' ? "unnamed" : filename + 1));

	if (pathlen >= sizeof(path)) {
		send_error_response(arg->fd, "file path too large");
		goto exit;
	}

	handle = curl_easy_init();
	if (handle == NULL) {
		send_error_response(arg->fd, "failed to initialize curl handle");
		goto exit;
	}

	if (download_file(handle, path, &arg->download, &code) == -1) {
		send_error_response(arg->fd,
			(code != CURLE_OK ? curl_easy_strerror(code) : strerror(errno)));
		goto curl_cleanup;
	}

	/* TODO: add file record */

	resp.status = RESPONSE_STATUS_OK;

	send(arg->fd, &resp, sizeof(resp), 0);

curl_cleanup:

	curl_easy_cleanup(handle);

exit:

	close(arg->fd);
	free(arg);

	pthread_exit(NULL);
}

static int handle_request_download(int fd, const struct request_data_download *download)
{
	struct download_routine_arg *arg;

	if ((arg = malloc(sizeof(*arg))) == NULL)
		return -1;

	arg->fd = fd;
	memcpy(&arg->download, download, sizeof(*download));

	if (create_detached_thread((void*(*)(void*))download_routine, arg) == -1) {
		free(arg);
		return -1;
	}

	return 0;
}

static void *show_info_routine(struct show_info_routine_arg *arg)
{
	close(arg->fd);
	free(arg);

	pthread_exit(NULL);
}

static int handle_request_show_info(int fd, struct request_data_show_info *show_info)
{
	struct show_info_routine_arg *arg;

	if ((arg = malloc(sizeof(*arg))) == NULL)
		return -1;

	arg->fd = fd;
	memcpy(&arg->show_info, show_info, sizeof(*show_info));

	if (create_detached_thread((void*(*)(void*))show_info_routine, arg) == -1) {
		free(arg);
		return -1;
	}

	return 0;
}

int daemon_handle_requests(int fd)
{
	struct sockaddr_un addr;
	socklen_t          addrlen = sizeof(addr);

	struct request req;

	ssize_t n;
	int     ret, newfd;

	while (1) {
		n = recvfrom(fd, &req, sizeof(req), 0, (struct sockaddr*)&addr, &addrlen);
		if (n == -1) {
			if (errno != EINTR)
				return -1;
			else
				continue;
		}

		puts(addr.sun_path);

		if ((ret = newfd = create_connected_socket(&addr)) != -1) {
			switch (req.type) {
			case REQUEST_TYPE_DOWNLOAD  :
				ret = handle_request_download(newfd, &req.data.download);
				break;

			case REQUEST_TYPE_SHOW_INFO :
				ret = handle_request_show_info(newfd, &req.data.show_info);
				break;

			default                     :
				close(newfd);
			}
		}

		if (ret == -1) {
			sendto_error_response(fd, &addr, strerror(errno));
			if (newfd != -1)
				close(newfd);
		}
	}

	return 0;
}
