/* $Id$ */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "syntax.h"
#include "fsm.h"
#include "out-dot.h"

extern int optind;

void usage(void) {
	printf("usage: fsm [-a] [<input> [<output>]]\n");
}

FILE *xopen(int argc, char * const argv[], int i, FILE *f, const char *mode) {
	if (argc <= i || 0 == strcmp("-", argv[i])) {
		return f;
	}

	f = fopen(argv[i], mode);
	if (f == NULL) {
		perror(argv[i]);
		exit(EXIT_FAILURE);
	}

	return f;
}

int main(int argc, char *argv[]) {
	struct state_list *l;
	struct fsm_state *start;
	FILE *in;
	FILE *out;

	static const struct fsm_options options_defaults;
	struct fsm_options options = options_defaults;

	{
		int c;

		while ((c = getopt(argc, argv, "ha")) != -1) {
			switch (c) {
			case 'h':
				usage();
				exit(EXIT_SUCCESS);

			case 'a':
				options.anonymous_states = 1;
				break;

			case '?':
			default:
				usage();
				exit(EXIT_FAILURE);
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (argc > 2) {
		usage();
		exit(EXIT_FAILURE);
	}

	in  = xopen(argc, argv, 0, stdin,  "r");
	out = xopen(argc, argv, 1, stdout, "w");

	parse(in, &l, &start);

	out_dot(out, &options, l, start);

	/* TODO: free stuff. make an api.c */

	return 0;
}

