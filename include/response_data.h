#ifndef RESPONSE_DATA_H
#define RESPONSE_DATA_H

#include "file_record.h"

struct response_data_download {
	/* while empty */
};

struct response_data_show_info {
	struct file_record file_record;
};

#endif // RESPONSE_DATA_H
