/* $Id$ */

#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/out.h>

#include "parser.h"

extern int optind;
extern char *optarg;

static void
usage(void)
{
	printf("usage: fsm    [-dmr] [-t <transformation>] [-x] {<text> ...}\n");
	printf("       fsm    [-dmr] [-t <transformation>] {-q <query>}\n");
	printf("       fsm -p [-dmr] [-t <transformation>] [-l <language>] [-acw] [-k <io>] [-e <prefix>]\n");
	printf("       fsm -h\n");
}

static enum fsm_io
io(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum fsm_io io;
	} a[] = {
		{ "getc", FSM_IO_GETC },
		{ "str",  FSM_IO_STR  },
		{ "pair", FSM_IO_PAIR }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].io;
		}
	}

	fprintf(stderr, "unrecognised IO API; valid IO APIs are: ");

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			i + 1 < sizeof a / sizeof *a ? ", " : "\n");
	}

	exit(EXIT_FAILURE);
}

static enum fsm_out
language(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum fsm_out language;
	} a[] = {
		{ "c",    FSM_OUT_C    },
		{ "csv",  FSM_OUT_CSV  },
		{ "dot",  FSM_OUT_DOT  },
		{ "fsm",  FSM_OUT_FSM  },
		{ "json", FSM_OUT_JSON }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].language;
		}
	}

	fprintf(stderr, "unrecognised output language; valid languages are: ");

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			i + 1 < sizeof a / sizeof *a ? ", " : "\n");
	}

	exit(EXIT_FAILURE);
}

static int
(*predicate(const char *name))
(const struct fsm *, const struct fsm_state *)
{
	size_t i;

	struct {
		const char *name;
		int (*predicate)(const struct fsm *, const struct fsm_state *);
	} a[] = {
		{ "isdfa",      fsm_isdfa      },
		{ "dfa",        fsm_isdfa      }
/* XXX:
		{ "iscomplete", fsm_iscomplete },
		{ "hasend",     fsm_hasend     },
		{ "end",        fsm_hasend     },
		{ "accept",     fsm_hasend     },
		{ "hasaccept",  fsm_hasend     }
*/
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].predicate;
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
/* XXX: needs predicate
		{ "complete",   fsm_complete    },
*/
		{ "complement",  fsm_complement  },
		{ "invert",      fsm_complement  },
		{ "reverse",     fsm_reverse     },
		{ "rev",         fsm_reverse     },
		{ "determinise", fsm_determinise },
		{ "dfa",         fsm_determinise },
		{ "todfa",       fsm_determinise },
		{ "min",         fsm_minimise    },
		{ "minimise",    fsm_minimise    }
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
		"complete, complement, reverse, determinise, minimise\n");
	exit(EXIT_FAILURE);
}

static FILE *
xopen(const char *s)
{
	FILE *f;

	assert(s != NULL);

	if (0 == strcmp(s, "-")) {
		return stdin;
	}

	f = fopen(s, "r");
	if (f == NULL) {
		perror(s);
		exit(EXIT_FAILURE);
	}

	return f;
}

int
main(int argc, char *argv[])
{
	enum fsm_out format;
	struct fsm *fsm;
	int xfiles;
	int print;
	int query;
	int r;

	static const struct fsm_outoptions o_defaults;
	struct fsm_outoptions o = o_defaults;

	o.comments = 1;
	o.io       = FSM_IO_GETC;

	format = FSM_OUT_FSM;
	xfiles = 0;
	print  = 0;
	query  = 0;

	/* XXX: makes no sense for e.g. fsm -h */
	fsm = fsm_parse(stdin);
	if (fsm == NULL) {
		exit(EXIT_FAILURE);
	}

	r = 0;

	{
		int c;

		while (c = getopt(argc, argv, "h" "acwe:k:" "xpq:l:dmrt:"), c != -1) {
			switch (c) {
			case 'a': o.anonymous_states  = 1;          break;
			case 'c': o.consolidate_edges = 1;          break;
			case 'w': o.fragment          = 1;          break;
			case 'e': o.prefix            = optarg;     break;
			case 'k': o.io                = io(optarg); break;

			case 'x': xfiles = 1;                       break;
			case 'p': print  = 1;                       break;
			case 'q': query  = 1;
			          r |= !fsm_all(fsm, predicate(optarg));
			          break;

			case 'l': format = language(optarg);        break;

			case 'd': transform(fsm, "determinise");    break;
			case 'm': transform(fsm, "minimise");       break;
			case 'r': transform(fsm, "reverse");        break;
			case 't': transform(fsm, optarg);           break;

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

	if ((argc > 0) + print + query > 1) {
		fprintf(stderr, "execute, -p and -q are mutually exclusive\n");
		return EXIT_FAILURE;
	}

	if (argc > 0) {
		int i;

		/* TODO: option to print input texts which match. like grep(1) does.
		 * This is not the same as printing patterns which match (by associating
		 * a pattern to the end state), like lx(1) does */

		for (i = 0; i < argc; i++) {
			const struct fsm_state *state;

			if (xfiles) {
				FILE *f;

				f = xopen(argv[0]);

				state = fsm_exec(fsm, fsm_fgetc, f);

				fclose(f);
			} else {
				const char *s;

				s = argv[i];

				state = fsm_exec(fsm, fsm_sgetc, &s);
			}

			if (state == NULL) {
				r |= 1;
				continue;
			}

			/* TODO: option to print state number? */
		}
	}

	if (print) {
		fsm_print(fsm, stdout, format, &o);
	}

	fsm_free(fsm);

	return r;
}

