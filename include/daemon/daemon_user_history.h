#ifndef DAEMON_USER_HISTORY_H
#define DAEMON_USER_HISTORY_H

#include "file_record.h"

struct daemon_user_history_node {
	struct file_record               record;
	struct daemon_user_history_node *next;
	struct daemon_user_history_node *prev;
};

struct daemon_user_history {
	struct daemon_user_history_node *head;
	struct daemon_user_history_node *tail;
	int                             cnt;
};

extern struct daemon_user_history user_history;

int daemon_user_history_push(const struct file_record *record);

#endif // DAEMON_USER_HISTORY_H:
