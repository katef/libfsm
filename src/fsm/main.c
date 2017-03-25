/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

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
#include <fsm/options.h>

#include "parser.h"

extern int optind;
extern char *optarg;

static struct fsm_options opt;

static void
usage(void)
{
	printf("usage: fsm [-x] {<text> ...}\n");
	printf("       fsm {-p} [-l <language>] [-acw] [-k <io>] [-e <prefix>]\n");
	printf("       fsm {-dmr | -t <transformation>} [<file>]\n");
	printf("       fsm {-q <query>} [<file>]\n");
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
		{ "api",  FSM_OUT_API  },
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

static int
(*transform(const char *name))
(struct fsm *)
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

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].f;
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
	int r;

	int (*query)(const struct fsm *, const struct fsm_state *);
	int (*uop)(struct fsm *);

	opt.comments = 1;
	opt.io       = FSM_IO_GETC;

	format = FSM_OUT_FSM;
	xfiles = 0;
	print  = 0;
	query  = NULL;
	uop    = NULL;

	fsm = NULL;
	r = 0;

	{
		int c;

		while (c = getopt(argc, argv, "h" "acwe:k:" "xpq:l:dmrt:"), c != -1) {
			switch (c) {
			case 'a': opt.anonymous_states  = 1;          break;
			case 'c': opt.consolidate_edges = 1;          break;
			case 'w': opt.fragment          = 1;          break;
			case 'e': opt.prefix            = optarg;     break;
			case 'k': opt.io                = io(optarg); break;

			case 'x': xfiles = 1;                         break;
			case 'p': print  = 1;                         break;
			case 'q': query  = predicate(optarg);         break;

			case 'l': format = language(optarg);          break;

			case 'd': uop = transform("determinise");     break;
			case 'm': uop = transform("minimise");        break;
			case 'r': uop = transform("reverse");         break;
			case 't': uop = transform(optarg);            break;

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

	if (!!uop + !!query > 1) {
		fprintf(stderr, "execute, -t and -q are mutually exclusive\n");
		return EXIT_FAILURE;
	}

#if 0
	if (binop != NULL) {
		if (argc != 2) {
			usage();
			return EXIT_FAILURE;
		}

		a = fsm_parse(xopen(argv[0]), &opt);
		if (a == NULL) {
			exit(EXIT_FAILURE);
		}

		b = fsm_parse(xopen(argv[1]), &opt);
		if (b == NULL) {
			exit(EXIT_FAILURE);
		}

		fsm = binop(a, b);
		if (fsm == NULL) {
			exit(EXIT_FAILURE);
		}
	}
#endif

	if (uop != NULL) {
		if (argc > 1) {
			usage();
			return EXIT_FAILURE;
		}

		fsm = fsm_parse(argc == 0 ? stdin : xopen(argv[0]), &opt);
		if (fsm == NULL) {
			exit(EXIT_FAILURE);
		}

		if (!uop(fsm)) {
			fprintf(stderr, "couldn't transform\n");
			exit(EXIT_FAILURE);
		}
	}

	if (fsm == NULL) {
		fsm = fsm_parse(stdin, &opt);
		if (fsm == NULL) {
			exit(EXIT_FAILURE);
		}
	}

	if (query != NULL) {
		r |= !fsm_all(fsm, query);
		return r;
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
		fsm_print(fsm, stdout, format);
	}

	fsm_free(fsm);

	return r;
}

