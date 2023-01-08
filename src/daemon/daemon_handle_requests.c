#include "request.h"
#include "response.h"

#include "daemon/daemon_user_histories.h"
#include "daemon/daemon_handle_requests.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <curl/curl.h>

#include <pwd.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <linux/limits.h>

struct download_routine_arg {
	struct request_data_download download;
	int                          pos;
	int                          fd;
};

struct show_info_routine_arg {
	struct request_data_show_info show_info;
	int                           pos;
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

static void set_error_response(struct response *resp, const char *strerror)
{
	resp->status = RESPONSE_STATUS_ERR;
	strncpy(resp->data.strerror, strerror, STRERROR_MAXLEN);
}

static void sendto_error_response(int fd, struct sockaddr_un *addr, const char *strerror)
{
	struct response resp;

	set_error_response(&resp, strerror);

	sendto(fd, &resp, sizeof(resp), 0, (struct sockaddr*)addr, sizeof(*addr));
}

static void send_error_response(int fd, const char *strerror)
{
	struct response resp;

	set_error_response(&resp, strerror);

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

static int uid_to_username(uid_t uid, char *username, int path_size)
{
	struct passwd *pwd = getpwuid(uid);

	if (pwd == NULL)
		return -1;

	snprintf(username, path_size, "%s", pwd->pw_name);

	return 0;
}

static int set_downloads_path(char *path, int path_size, const char *username)
{
	const char *fmt = (strcmp(username, "root") == 0 ? "/%s/%s" : "/home/%s/%s");
	return snprintf(path, path_size, fmt, username, "Downloads");
}

static int set_filename_from_url(char *filename, int filename_size, const char *url)
{
	const char *ptr = strrchr(url, '/');
	ptr = (ptr == NULL || ptr[1] == '\0' ? "unnamed" : ptr + 1);
	return snprintf(filename, filename_size, "/%s", ptr);
}

static int fill_file_record(struct file_record *record, const char *path, 
	struct download_routine_arg *arg, CURL *handle)
{
	struct stat statbuf;

	if (stat(path, &statbuf) == -1)
		return -1;

	strncpy(record->url, arg->download.url, sizeof(record->url));

	return 0;
}

static void *download_routine(struct download_routine_arg *arg)
{
	CURL     *handle;
	CURLcode code = CURLE_OK;

	struct response    resp;
	struct file_record record;

	int  len;
	char path[PATH_MAX], username[NAME_MAX];
	
	if (uid_to_username(user_histories.hm[arg->pos]->uid, username, sizeof(username)) == -1) {
		send_error_response(arg->fd, strerror(errno));
		goto exit;
	}

	len = set_downloads_path(path, sizeof(path), username);
	if (create_downloads_directory(path) == -1) {
		send_error_response(arg->fd, strerror(errno));
		goto exit;
	}

	len += set_filename_from_url(path + len, sizeof(path) - len, arg->download.url);
	if (len >= sizeof(path)) {
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

	if (fill_file_record(&record, path, arg, handle) == -1) {
		send_error_response(arg->fd, strerror(errno));
		goto curl_cleanup;
	}

	if (daemon_user_histories_push_record(arg->pos, &record) == -1) {
		send_error_response(arg->fd, strerror(errno));
		goto curl_cleanup;
	}

	resp.status = RESPONSE_STATUS_OK;

	send(arg->fd, &resp, sizeof(resp), 0);

curl_cleanup:

	curl_easy_cleanup(handle);

exit:

	close(arg->fd);
	free(arg);

	pthread_exit(NULL);
}

static int handle_request_download(int fd, int pos, const struct request_data_download *download)
{
	struct download_routine_arg *arg;

	if ((arg = malloc(sizeof(*arg))) == NULL)
		return -1;

	arg->fd  = fd;
	arg->pos = pos;
	memcpy(&arg->download, download, sizeof(*download));

	if (create_detached_thread((void*(*)(void*))download_routine, arg) == -1) {
		free(arg);
		return -1;
	}

	return 0;
}

static void *show_info_routine(struct show_info_routine_arg *arg)
{
	struct daemon_user_history_node *node = user_histories.hm[arg->pos]->head;
	struct response                 resp;

	while (node != NULL) {
		resp.status = RESPONSE_STATUS_OK;
		memcpy(&resp.data.show_info.record, &node->record, sizeof(node->record));

		if (send(arg->fd, &resp, sizeof(resp), 0) == -1) {
			if (errno != EINTR)
				break;
			else
				continue;
		}

		node = node->next;
	}

	resp.status = RESPONSE_STATUS_STOP;

	send(arg->fd, &resp, sizeof(resp), 0);

	close(arg->fd);
	free(arg);

	pthread_exit(NULL);
}

static int handle_request_show_info(int fd, int pos, struct request_data_show_info *show_info)
{
	struct show_info_routine_arg *arg;

	if ((arg = malloc(sizeof(*arg))) == NULL)
		return -1;

	arg->fd  = fd;
	arg->pos = pos;
	memcpy(&arg->show_info, show_info, sizeof(*show_info));

	if (create_detached_thread((void*(*)(void*))show_info_routine, arg) == -1) {
		free(arg);
		return -1;
	}

	return 0;
}

static int get_uid(uid_t *uid, const char *socket_path)
{
	struct stat statbuf;

	if (stat(socket_path, &statbuf) == -1)
		return -1;

	*uid = statbuf.st_uid;

	return 0;
}

int daemon_handle_requests(int fd)
{
	struct sockaddr_un addr;
	socklen_t          addrlen = sizeof(addr);

	struct request req;

	ssize_t n;
	int     ret, newfd, pos;

	uid_t uid;

	while (1) {
		n = recvfrom(fd, &req, sizeof(req), 0, (struct sockaddr*)&addr, &addrlen);
		if (n == -1) {
			if (errno != EINTR)
				return -1;
			else
				continue;
		}

		if (get_uid(&uid, addr.sun_path) == -1) {
			sendto_error_response(fd, &addr, strerror(errno));
			continue;
		}

		pos = daemon_user_histories_insert_by_uid(uid);
		if (pos == -1) {
			sendto_error_response(fd, &addr,
				(errno > 0 ? strerror(errno) : "user histories storage has no space"));
			continue;
		}

		if ((ret = newfd = create_connected_socket(&addr)) != -1) {
			switch (req.type) {
			case REQUEST_TYPE_DOWNLOAD  :
				ret = handle_request_download(newfd, pos, &req.data.download);
				break;

			case REQUEST_TYPE_SHOW_INFO :
				ret = handle_request_show_info(newfd, pos, &req.data.show_info);
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
