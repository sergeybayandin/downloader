#ifndef REQUEST_DATA_H
#define REQUEST_DATA_H

#include "config.h"

struct request_data_download {
	char url[URL_MAXLEN];
	char login[LOGIN_MAXLEN];
	char password[PASSWORD_MAXLEN];
};

struct request_data_show_info {
	/* while empty */
};

#endif // REQUEST_DATA_H
