#ifndef DAEMON_USER_HISTORIES_H
#define DAEMON_USER_HISTORIES_H

#include "file_record.h"

#include <linux/limits.h>

struct daemon_user_history_node {
	struct file_record              record;

	struct daemon_user_history_node *next;
	struct daemon_user_history_node *prev;
};

struct daemon_user_history {
	struct daemon_user_history_node *head;
	struct daemon_user_history_node *tail;

	uid_t                           uid;
	int                             cnt;
};

struct daemon_user_histories {
	struct daemon_user_history **hm;
	int                        cnt;
};

extern struct daemon_user_histories user_histories;

int  daemon_user_histories_insert_by_uid(uid_t uid);
void daemon_user_histories_free(void);

int  daemon_user_histories_push_record(int pos, const struct file_record *record);
void daemon_user_histories_free_history(int pos);

#endif // DAEMON_USER_HISTORIES_H
