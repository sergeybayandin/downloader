#ifndef RESPONSE_H
#define RESPONSE_H

#include "response_data.h"

#define RESPONSE_STATUS_OK  0
#define RESPONSE_STATUS_ERR 1

#define RESPONSE_STATUS_STOP 2

struct response {
	int status;
	union {
		struct response_data_download  download;
		struct response_data_show_info show_info;
		char                           strerror[STRERROR_MAXLEN];
	}data;
};

#endif // RESPONSE_H
