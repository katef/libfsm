/* $Id$ */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/parse.h>
#include <fsm/graph.h>
#include <fsm/out.h>

extern int optind;
extern char *optarg;

void usage(void) {
	printf("usage: fsm [-adrm] [<input> [<output>]]\n");
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
	FILE *in;
	FILE *out;
	enum fsm_out format = FSM_OUT_FSM;

	struct cli_options {
		/* boolean: reverse the FSM as per fsm_reverse() */
		int reverse:1;

		/* boolean: convert to a DFA as per fsm_todfa() */
		int todfa:1;

		/* boolean: minimize redundant transitions */
		int minimize:1;
	};

	static const struct fsm_options options_defaults;
	struct fsm_options options = options_defaults;

	static const struct cli_options cli_options_defaults;
	struct cli_options cli_options = cli_options_defaults;

	{
		int c;

		while ((c = getopt(argc, argv, "hal:drm")) != -1) {
			switch (c) {
			case 'h':
				usage();
				exit(EXIT_SUCCESS);

			case 'a':
				options.anonymous_states = 1;
				break;

			case 'm':
				cli_options.minimize = 1;
				break;

			case 'l':
				if (0 == strcmp(optarg, "fsm")) {
					format = FSM_OUT_FSM;
				} else if (0 == strcmp(optarg, "dot")) {
					format = FSM_OUT_DOT;
				} else if (0 == strcmp(optarg, "table")) {
					format = FSM_OUT_TABLE;
				} else if (0 == strcmp(optarg, "c")) {
					format = FSM_OUT_C;
				} else {
					fprintf(stderr, "unrecognised output language; valid languages are: fsm, dot, table, c\n");
					exit(EXIT_FAILURE);
				}
				break;

			case 'd':
				cli_options.todfa = 1;
				break;

			case 'r':
				cli_options.reverse = 1;
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

	{
		struct fsm *fsm;

		fsm = fsm_new();
		if (fsm == NULL) {
			perror("fsm_new");
			exit(EXIT_FAILURE);
		}

		fsm_setoptions(fsm, &options);

		fsm_parse(fsm, in);

		if (cli_options.reverse) {
			if (fsm_reverse(fsm) == NULL) {
				perror("fsm_reverse");
				exit(EXIT_FAILURE);
			}
		}

		if (cli_options.todfa) {
			if (fsm_todfa(fsm) == NULL) {
				perror("fsm_todfa");
				exit(EXIT_FAILURE);
			}
		}

		if (cli_options.minimize) {
			if (fsm_minimize(fsm) == NULL) {
				perror("fsm_minimize");
				exit(EXIT_FAILURE);
			}
		}

		fsm_print(fsm, out, format);

		fsm_free(fsm);
	}

	return 0;
}

