#include "daemon/daemon_user_history.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct daemon_user_history user_history = {
	.head = NULL, .tail = NULL, .cnt = 0
};

struct daemon_user_history_node *
create_node(const struct file_record *record)
{
	struct daemon_user_history_node *node;

	node = calloc(1, sizeof(*node));
	if (node == NULL)
		return NULL;

	memcpy(&node->record, record, sizeof(*record));

	return node;
}

int daemon_user_history_push(const struct file_record *record)
{
	struct daemon_user_history_node *node = create_node(record);

	if (node == NULL)
		return -1;

	if (user_history.head != NULL) {
		user_history.tail->next = node;
		node->prev              = user_history.tail;
	} else {
		user_history.head = user_history.tail = node;
	}

	++user_history.cnt;

	return 0;
}
