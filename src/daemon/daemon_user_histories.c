#include "config.h"

#include "daemon/daemon_user_histories.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct daemon_user_histories user_histories = {.hm = NULL, .cnt = 0};

static int hash(uid_t uid)
{
	return uid % USER_HISTORIES_MAXCNT;
}

static int create_user_histories(void)
{
	void *ptr;

	ptr = calloc(USER_HISTORIES_MAXCNT, sizeof(*user_histories.hm));
	if (ptr == NULL)
		return -1;

	user_histories.hm = ptr;

	return 0;
}

static int find(uid_t uid)
{
	int i = -1;

	if (user_histories.hm != NULL) {
		int                        pos;
		struct daemon_user_history **hm = user_histories.hm;

		i = pos = hash(uid);
	
		while (hm[i] != NULL) {
			if (hm[i]->uid == uid)
				return i;

			++i;

			if (i >= USER_HISTORIES_MAXCNT)
				i %= USER_HISTORIES_MAXCNT;

			if (i == pos)
				return -1;
		}
	}

	return i;
}

int daemon_user_histories_insert_by_uid(uid_t uid)
{
	if (user_histories.hm == NULL && create_user_histories() == -1)
		return -1;

	struct daemon_user_history **hm = user_histories.hm;
	int                        pos  = find(uid);

	if (pos == -1)
		return -1;

	if (hm[pos] == NULL) {
		if ((hm[pos] = calloc(1, sizeof(*hm[pos]))) == NULL)
			return -1;
		hm[pos]->uid = uid;
		++user_histories.cnt;
	}

	return pos;
}

void daemon_user_histories_free(void)
{
	if (user_histories.hm != NULL) {
		int i;

		for (i = 0; i < USER_HISTORIES_MAXCNT; ++i)
			daemon_user_histories_free_history(i);

		free(user_histories.hm);
		memset(&user_histories, 0, sizeof(user_histories));
	}
}

static struct daemon_user_history_node *
create_node(const struct file_record *record)
{
	struct daemon_user_history_node *node;

	if ((node = calloc(1, sizeof(*node))) == NULL)
		return NULL;

	memcpy(&node->record, record, sizeof(*record));

	return node;
}

int daemon_user_histories_push_record(int pos, const struct file_record *record)
{
	assert(pos >= 0 && pos < USER_HISTORIES_MAXCNT);

	struct daemon_user_history_node *node;
	struct daemon_user_history      *user_history = user_histories.hm[pos];

	if ((node = create_node(record)) == NULL) 
		return -1;

	if (user_history->head != NULL) {
		user_history->tail->next = node;
		node->prev               = user_history->tail;
		user_history->tail       = node;
	} else {
		user_history->tail = user_history->head = node;
	}

	++user_history->cnt;

	return 0;
}

void daemon_user_histories_free_history(int pos)
{
	assert(pos >= 0 && pos < USER_HISTORIES_MAXCNT);

	if (user_histories.hm[pos] == NULL)
		return;

	struct daemon_user_history      *user_history = user_histories.hm[pos];
	struct daemon_user_history_node *node         = user_history->head, *next;

	while (node != NULL) {
		next = node->next;
		free(node);
		node = next;
	}

	free(user_history);
	user_histories.hm[pos] = NULL;
}
