#ifndef FILE_RECORD_H
#define FILE_RECORD_H

#include "config.h"

#include <sys/types.h>

struct file_record {
	char   url[URL_MAXLEN];
	off_t  size;
	time_t date;
};

#endif // FILE_RECORD_H
