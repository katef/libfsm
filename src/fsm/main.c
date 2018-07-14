/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 200112L

#include <unistd.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include "parser.h"

extern int optind;
extern char *optarg;

static struct fsm_options opt;

#define OP_ARITY 0x1

enum op {
	/* unary */
	OP_IDENTITY    = ( 0 << 1) | 1,
	OP_COMPLETE    = ( 1 << 1) | 1,
	OP_COMPLEMENT  = ( 2 << 1) | 1,
	OP_REVERSE     = ( 3 << 1) | 1,
	OP_DETERMINISE = ( 4 << 1) | 1,
	OP_MINIMISE    = ( 5 << 1) | 1,
	OP_TRIM        = ( 6 << 1) | 1,

	/* binary */
	OP_CONCAT      = ( 7 << 1) | 0,
	OP_UNION       = ( 8 << 1) | 0,
	OP_INTERSECT   = ( 9 << 1) | 0,
	OP_SUBTRACT    = (10 << 1) | 0,
	OP_EQUAL       = (11 << 1) | 0
};

static int
query_countstates(const struct fsm *fsm, const struct fsm_state *state)
{
	(void) fsm;
	(void) state;

	/* never called */
	abort();
}

static void
usage(void)
{
	printf("usage: fsm [-x] {<text> ...}\n");
	printf("       fsm {-p} [-l <language>] [-acwX] [-k <io>] [-e <prefix>]\n");
	printf("       fsm {-dmr | -t <transformation>} [-i <iterations>] [<file.fsm> | <file-a> <file-b>]\n");
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

static fsm_print *
print_name(const char *name)
{
	size_t i;

	struct {
		const char *name;
		fsm_print *f;
	} a[] = {
		{ "api",  fsm_print_api  },
		{ "c",    fsm_print_c    },
		{ "dot",  fsm_print_dot  },
		{ "fsm",  fsm_print_fsm  },
		{ "ir",   fsm_print_ir   },
		{ "json", fsm_print_json }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].f;
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
(*query_name(const char *name,
	int (**walk)(const struct fsm *,
		int (*)(const struct fsm *, const struct fsm_state *))))
(const struct fsm *, const struct fsm_state *)
{
	size_t i;

	/*
	 * These queries apply to an FSM as a whole.
	 * fsm_reachableall/any() equivalents are not included here,
	 * because the same effect may be achieved by OP_TRIM first.
	 */

	struct {
		const char *name;
		int (*walk)(const struct fsm *,
			int (*)(const struct fsm *, const struct fsm_state *));
		int (*pred)(const struct fsm *, const struct fsm_state *);
	} a[] = {
		{ "isdfa",      fsm_all, fsm_isdfa         },
		{ "dfa",        fsm_all, fsm_isdfa         },
		{ "count",      NULL,    query_countstates },
		{ "iscomplete", fsm_all, fsm_iscomplete    },
		{ "hasend",     fsm_has, fsm_isend         },
		{ "end",        fsm_has, fsm_isend         },
		{ "accept",     fsm_has, fsm_isend         },
		{ "hasaccept",  fsm_has, fsm_isend         }
	};

	assert(name != NULL);
	assert(walk != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			*walk = a[i].walk;
			return a[i].pred;
		}
	}

	fprintf(stderr, "unrecognised query; valid queries are: ");

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			i + 1 < sizeof a / sizeof *a ? ", " : "\n");
	}

	exit(EXIT_FAILURE);
}

static enum op
op_name(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum op op;
	} a[] = {
		{ "complete",    OP_COMPLETE    },

		{ "complement",  OP_COMPLEMENT  },
		{ "invert",      OP_COMPLEMENT  },
		{ "reverse",     OP_REVERSE     },
		{ "rev",         OP_REVERSE     },
		{ "determinise", OP_DETERMINISE },
		{ "dfa",         OP_DETERMINISE },
		{ "todfa",       OP_DETERMINISE },
		{ "min",         OP_MINIMISE    },
		{ "minimise",    OP_MINIMISE    },
		{ "trim",        OP_TRIM        },

		{ "cat",         OP_CONCAT      },
		{ "concat",      OP_CONCAT      },
		{ "union",       OP_UNION       },
		{ "intersect",   OP_INTERSECT   },
		{ "subtract",    OP_SUBTRACT    },
		{ "sub",         OP_SUBTRACT    },
		{ "minus",       OP_SUBTRACT    },
		{ "equals",      OP_EQUAL       },
		{ "equal",       OP_EQUAL       }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].op;
		}
	}

	fprintf(stderr, "unrecognised operation; valid operations are: ");

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			i + 1 < sizeof a / sizeof *a ? ", " : "\n");
	}

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
	unsigned iterations, i;
	double elapsed;
	fsm_print *print;
	enum op op;
	struct fsm *fsm;
	int xfiles;
	int r;

	int (*query)(const struct fsm *, const struct fsm_state *);
	int (*walk )(const struct fsm *,
		 int (*)(const struct fsm *, const struct fsm_state *));

	opt.comments = 1;
	opt.io       = FSM_IO_GETC;

	xfiles = 0;
	print  = NULL;
	query  = NULL;
	walk   = NULL;
	op     = OP_IDENTITY;

	iterations = 1;
	fsm = NULL;
	r = 0;

	{
		int c;

		while (c = getopt(argc, argv, "h" "acwXe:k:i:" "xpq:l:dmrt:"), c != -1) {
			switch (c) {
			case 'a': opt.anonymous_states  = 1;          break;
			case 'c': opt.consolidate_edges = 1;          break;
			case 'w': opt.fragment          = 1;          break;
			case 'X': opt.always_hex        = 1;          break;
			case 'e': opt.prefix            = optarg;     break;
			case 'k': opt.io                = io(optarg); break;

			case 'i':
				iterations = strtoul(optarg, NULL, 10);
				/* XXX: error handling */
				break;

			case 'x': xfiles = 1;                         break;
			case 'l': print  = print_name(optarg);        break;
			case 'p': print  = fsm_print_fsm;             break;
			case 'q': query  = query_name(optarg, &walk); break;

			case 'd': op = op_name("determinise");        break;
			case 'm': op = op_name("minimise");           break;
			case 'r': op = op_name("reverse");            break;
			case 't': op = op_name(optarg);               break;

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

	if ((op != OP_IDENTITY) + !!query > 1) {
		fprintf(stderr, "execute, -t and -q are mutually exclusive\n");
		return EXIT_FAILURE;
	}

	if ((op == OP_EQUAL) + !!print > 1) {
		fprintf(stderr, "-t equal and -p are mutually exclusive\n");
		return EXIT_FAILURE;
	}

	elapsed = 0.0;

	for (i = 0; i < iterations; i++) {
		struct timespec pre, post;
		struct fsm *a, *b;
		struct fsm *q;

		if ((op & OP_ARITY) == 1) {
			if (argc > 1) {
				usage();
				exit(EXIT_FAILURE);
			}

			q = fsm_parse((argc == 0) ? stdin : xopen(argv[0]), &opt);
			if (q == NULL) {
				exit(EXIT_FAILURE);
			}
		} else {
			if (argc != 2) {
				usage();
				exit(EXIT_FAILURE);
			}

			a = fsm_parse(xopen(argv[0]), &opt);
			if (a == NULL) {
				exit(EXIT_FAILURE);
			}

			b = fsm_parse(xopen(argv[1]), &opt);
			if (b == NULL) {
				exit(EXIT_FAILURE);
			}
		}

		if (-1 == clock_gettime(CLOCK_MONOTONIC, &pre)) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		/* within this block, r is API convention (true for success) */
		r = 1;

		switch (op) {
		case OP_IDENTITY:
			break;

		case OP_COMPLETE:    r = fsm_complete(q, fsm_isany); break;

		case OP_COMPLEMENT:  r = fsm_complement(q);   break;
		case OP_REVERSE:     r = fsm_reverse(q);      break;
		case OP_DETERMINISE: r = fsm_determinise(q);  break;
		case OP_MINIMISE:    r = fsm_minimise(q);     break;
		case OP_TRIM:        r = fsm_trim(q);         break;

		case OP_CONCAT:      q = fsm_concat(a, b);    break;
		case OP_UNION:       q = fsm_union(a, b);     break;
		case OP_INTERSECT:   q = fsm_intersect(a, b); break;
		case OP_SUBTRACT:    q = fsm_subtract(a, b);  break;

		case OP_EQUAL:
			r = fsm_equal(a, b);
			fsm_free(a);
			fsm_free(b);
			q = NULL;
			break;

		default:
			fprintf(stderr, "unrecognised operation\n");
			exit(EXIT_FAILURE);
		}

		if (r == -1) {
			q = NULL;
		}

		if (-1 == clock_gettime(CLOCK_MONOTONIC, &post)) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		if (iterations > 1) {
			double ms;

			ms = 1000.0 * (post.tv_sec  - pre.tv_sec)
			            + (post.tv_nsec - pre.tv_nsec) / 1e6;
			elapsed += ms;

			printf("%f ", ms);
		}

		if (op != OP_EQUAL && q == NULL) {
			perror("fsm_op");
			exit(EXIT_FAILURE);
		}

		r = !r;

		fsm = q;
	}

	if (iterations > 1) {
		printf("=> total %g ms (avg %g ms)\n", elapsed, elapsed / iterations);
	}

	/* henceforth, r is $?-convention (0 for success) */

	if (fsm == NULL) {
		return r;
	}

	/* TODO: OP_EQUAL ought to have the same CLI interface as a predicate */
	if (query != NULL) {
		/* TODO: benchmark */
		/* XXX: this symbol is used like an enum here. It's a bit of a kludge */
		if (query == query_countstates) {
			assert(walk == NULL);
			printf("%u\n", fsm_countstates(fsm));
			return 0;
		} else {
			assert(walk != NULL);
			r |= !walk(fsm, query);
			return r;
		}
	}

	/* TODO: optional -- to delimit texts as opposed to .fsm filenames */
	if (op == OP_IDENTITY && argc > 0) {
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

	if (print != NULL) {
		print(stdout, fsm);
	}

	fsm_free(fsm);

	return r;
}

