#ifndef CLI_PARSE_ARGS_H
#define CLI_PARSE_ARGS_H

struct cli_parsed_args {
	const char *url;

	const char *login;
	const char *password;

	int         has_show_info;
};

extern struct cli_parsed_args pa;

int cli_parse_args(int argc, char *argv[]);

#endif // CLI_PARSE_ARGS_H
