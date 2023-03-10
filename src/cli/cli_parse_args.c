#include "config.h"

#include "cli/cli_parse_args.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>

struct cli_parsed_args pa = {
	.url           = NULL,
	.login         = NULL,
	.password      = NULL,
	.has_show_info = 0
};

static void print_usage(const char *prog)
{
	fprintf(stderr, "Usage: %s <option> <option>...\n", prog);
	fprintf(stderr, "\n");
	fprintf(stderr, "Where <option> is any of the following options:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "    --url       <arg> [url to download file]\n");
	fprintf(stderr, "    --login     <arg> []\n");
	fprintf(stderr, "    --password  <arg> []\n");
	fprintf(stderr, "    --show-info <arg> []\n");
}

int cli_parse_args(int argc, char *argv[])
{
	assert(argc >= 1);
	assert(argv != NULL);

	const struct option options[] = {
		{"url",       required_argument, NULL, 0},
		{"login",     required_argument, NULL, 1},
		{"password",  required_argument, NULL, 2},
		{"show-info", no_argument,       NULL, 3},
		{NULL,        0,                 NULL, 0}
	};

	int opt;

	memset(&pa, 0, sizeof(pa));

	while (1) {
		opt = getopt_long(argc, argv, "", options, NULL);
		if (opt == -1)
			break;

		switch (opt) {
		case 0  :
			if (strlen(optarg) >= URL_MAXLEN) {
				fprintf(stderr, "cli_parse_args: url max length must be less than %d\n", URL_MAXLEN);
				return -1;
			}
			pa.url = optarg;
			break;

		case 1  :
			if (strlen(optarg) >= LOGIN_MAXLEN) {
				fprintf(stderr, "cli_parse_args: login max length must be less than %d\n", LOGIN_MAXLEN);
				return -1;
			}
			pa.login = optarg;
			break;

		case 2  :
			if (strlen(optarg) >= PASSWORD_MAXLEN) {
				fprintf(stderr, "cli_parse_args: password max length must be less than %d\n", PASSWORD_MAXLEN);
				return -1;
			}
			pa.password = optarg;
			break;

		case 3  :
			pa.has_show_info = 1;
			break;

		default :
			print_usage(argv[0]);
			return -1;
		}
	}

	if ((pa.url == NULL && !pa.has_show_info) ||
	    (pa.url == NULL && (pa.login != NULL || pa.password != NULL))) {
		print_usage(argv[0]);
		return -1;
	}

	return 0;
}
