#ifndef PARSE_ARGS_H
#define PARSE_ARGS_H

struct parsed_args {
	const char *url;

	const char *login;
	const char *password;

	int         has_show_info;
};

extern struct parsed_args pa;

int  parse_args(int argc, char *argv[]);
void print_usage(const char *prog);

#endif // PARSE_ARGS_H
