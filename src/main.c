/* $Id$ */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "syntax.h"
#include "fsm.h"
#include "out.h"

extern int optind;
extern char *optarg;

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
	struct state_list *sl;
	struct label_list *ll;
	struct fsm_state *start;
	FILE *in;
	FILE *out;
	void (*outf)(FILE *f, const struct fsm_options *options,
		struct state_list *sl, struct label_list *ll, struct fsm_state *start);

	static const struct fsm_options options_defaults;
	struct fsm_options options = options_defaults;

	outf = out_fsm;

	{
		int c;

		while ((c = getopt(argc, argv, "hal:")) != -1) {
			switch (c) {
			case 'h':
				usage();
				exit(EXIT_SUCCESS);

			case 'a':
				options.anonymous_states = 1;
				break;

			case 'l':
				if (0 == strcmp(optarg, "fsm")) {
					outf = out_fsm;
				} else if (0 == strcmp(optarg, "dot")) {
					outf = out_dot;
				} else if (0 == strcmp(optarg, "table")) {
					outf = out_table;
				} else {
					fprintf(stderr, "unrecognised output language; valid languages are: fsm, dot, table\n");
					exit(EXIT_FAILURE);
				}
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

	parse(in, &sl, &ll, &start);

	outf(out, &options, sl, ll, start);

	/* TODO: free stuff. make an api.c */

	return 0;
}

