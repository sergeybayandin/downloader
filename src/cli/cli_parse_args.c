#include "config.h"

#include "cli/cli_parse_args.h"

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
	// TODO
}

int cli_parse_args(int argc, char *argv[])
{
	if (argc < 1 || argv == NULL)
		return -1;

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

		switch (opt) {
		case 0  :
			if (strlen(optarg) >= URL_MAXLEN) {
				fprintf(stderr, "parse_args: url max length must be less than %d\n", URL_MAXLEN);
				return -1;
			}
			pa.url = optarg;
			break;

		case 1  :
			if (strlen(optarg) >= LOGIN_MAXLEN) {
				fprintf(stderr, "parse_args: login max length must be less than %d\n", LOGIN_MAXLEN);
				return -1;
			}
			pa.login = optarg;
			break;

		case 2  :
			if (strlen(optarg) >= PASSWORD_MAXLEN) {
				fprintf(stderr, "parse_args: password max length must be less than %d\n", PASSWORD_MAXLEN);
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

	return 0;
}
