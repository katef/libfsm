/* $Id$ */

#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/graph.h>
#include <fsm/out.h>
#include <fsm/exec.h>

#include "parser.h"

extern int optind;
extern char *optarg;

FILE *out;

static void
usage(void)
{
	printf("usage: fsm [-chagdpmr] [-o <file> ] [-P <file>] [-l <language>] [-n <prefix>]\n"
	       "           [-t <transformation>] [-e <execution> | -q <query>]\n");
}

static int
query(struct fsm *fsm, const char *name)
{
	size_t i;

	struct {
		const char *name;
		int (*f)(const struct fsm *);
	} a[] = {
		{ "iscomplete", fsm_iscomplete },
		{ "isdfa",      fsm_isdfa      },
		{ "dfa",        fsm_isdfa      },
		{ "hasend",     fsm_hasend     },
		{ "end",        fsm_hasend     },
		{ "accept",     fsm_hasend     },
		{ "hasaccept",  fsm_hasend     }
	};

	assert(fsm != NULL);
	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].f(fsm);
		}
	}

	fprintf(stderr, "unrecognised query; valid queries are: "
		"iscomplete, isdfa, hasend\n");
	exit(EXIT_FAILURE);
}

static void
transform(struct fsm *fsm, const char *name)
{
	size_t i;

	struct {
		const char *name;
		int (*f)(struct fsm *);
	} a[] = {
		{ "complete",   fsm_complete   },
		{ "complement", fsm_complement },
		{ "invert",     fsm_complement },
		{ "reverse",    fsm_reverse    },
		{ "rev",        fsm_reverse    },
		{ "dfa",        fsm_todfa      },
		{ "todfa",      fsm_todfa      },
		{ "min",        fsm_minimize   },
		{ "minimize",   fsm_minimize   }
	};

	assert(fsm != NULL);
	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			if (!a[i].f(fsm)) {
				fprintf(stderr, "couldn't transform\n");
				exit(EXIT_FAILURE);
			}
			return;
		}
	}

	fprintf(stderr, "unrecognised transformation; valid transformations are: "
		"complete, complement, reverse, todfa, minimize\n");
	exit(EXIT_FAILURE);
}

static enum fsm_out
language(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum fsm_out format;
	} a[] = {
		{ "fsm",   FSM_OUT_FSM   },
		{ "dot",   FSM_OUT_DOT   },
		{ "table", FSM_OUT_TABLE },
		{ "c",     FSM_OUT_C     }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].format;
		}
	}

	fprintf(stderr, "unrecognised output language; valid languages are: "
		"fsm, dot, table, c\n");
	exit(EXIT_FAILURE);
}

static void
cleanup(void)
{
	if (out != NULL) {
		fclose(out);
	}
}

int
main(int argc, char *argv[])
{
	enum fsm_out format = FSM_OUT_FSM;
	struct fsm *fsm;

	static const struct fsm_outoptions outoptions_defaults;
	struct fsm_outoptions out_options = outoptions_defaults;

	/* XXX: makes no sense for e.g. fsm -h */
	fsm = fsm_parse(stdin);
	if (fsm == NULL) {
		exit(EXIT_FAILURE);
	}

	atexit(cleanup);

	{
		int c;

		while ((c = getopt(argc, argv, "hagn:l:de:o:cmrt:pP:q:")) != -1) {
			switch (c) {
			case 'a': out_options.anonymous_states  = 1;      break;
			case 'g': out_options.fragment          = 1;      break;
			case 'c': out_options.consolidate_edges = 1;      break;
			case 'n': out_options.prefix            = optarg; break;

			case 'e': return fsm_exec(fsm, fsm_sgetc, &optarg);

			case 'P':
				{
					FILE *f;

					f = 0 == strcmp(optarg, "-") ? stdout : fopen(optarg, "w");
					if (f == NULL) {
						perror(optarg);
						exit(EXIT_FAILURE);
					}

					fsm_print(fsm, f, format, &out_options);

					fclose(f);
				}
				break;

			case 'p':
				out = stdout;
				break;

			case 'o':
				out = 0 == strcmp(optarg, "-") ? stdout : fopen(optarg, "w");
				if (out == NULL) {
					perror(optarg);
					exit(EXIT_FAILURE);
				}
				break;

			case 'l': format = language(optarg);  break;

			case 'd': transform(fsm, "todfa");    break;
			case 'm': transform(fsm, "minimize"); break;
			case 'r': transform(fsm, "reverse");  break;
			case 't': transform(fsm, optarg);     break;

			case 'q': exit(query(fsm, optarg));   break;

			case 'h':
				usage();
				exit(EXIT_SUCCESS);

			case '?':
			default:
				usage();
				exit(EXIT_FAILURE);
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (argc != 0) {
		usage();
		exit(EXIT_FAILURE);
	}

	if (out != NULL) {
		fsm_print(fsm, out, format, &out_options);
	}

	fsm_free(fsm);

	return 0;
}

