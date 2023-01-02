#ifndef REQUEST_H
#define REQUEST_H

#include "request_data.h"

#define REQUEST_TYPE_DOWNLOAD  0
#define REQUEST_TYPE_SHOW_INFO 1

struct request {
	int type;
	union {
		struct request_data_download  download;
		struct request_data_show_info show_info;
	}data;
};

#endif // REQUEST_H
