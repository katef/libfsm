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
	OP_EQUAL       = (11 << 1) | 0,
	OP_SUBTRACT2   = (12 << 1) | 0
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
	printf("       fsm {-dmr | -t <transformation>} [<file.fsm> | <file-a> <file-b>]\n");
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
		{ "isdfa",      fsm_isdfa         },
		{ "dfa",        fsm_isdfa         },
		{ "count",      query_countstates }
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
		"iscomplete, isdfa, hasend, count\n");
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
		{ "equal",       OP_EQUAL       },
		{ "subtract2",   OP_SUBTRACT2   }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].op;
		}
	}

	fprintf(stderr, "unrecognised operation; valid operations are: "
		"complete, "
		"complement, invert, reverse, determinise, minimise, trim, "
		"concat, union, intersect, subtract, equal\n");
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
	enum op op;
	struct fsm *fsm;
	int xfiles;
	int print;
	int r;

	int (*query)(const struct fsm *, const struct fsm_state *);

	opt.comments = 1;
	opt.io       = FSM_IO_GETC;

	format = FSM_OUT_FSM;
	xfiles = 0;
	print  = 0;
	query  = NULL;
	op     = OP_IDENTITY;

	fsm = NULL;
	r = 0;

	{
		int c;

		while (c = getopt(argc, argv, "h" "acwXe:k:" "xpq:l:dmrt:"), c != -1) {
			switch (c) {
			case 'a': opt.anonymous_states  = 1;          break;
			case 'c': opt.consolidate_edges = 1;          break;
			case 'w': opt.fragment          = 1;          break;
			case 'X': opt.always_hex        = 1;          break;
			case 'e': opt.prefix            = optarg;     break;
			case 'k': opt.io                = io(optarg); break;

			case 'x': xfiles = 1;                         break;
			case 'p': print  = 1;                         break;
			case 'q': query  = predicate(optarg);         break;

			case 'l': format = language(optarg);          break;

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

	{
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
		case OP_SUBTRACT2:   q = fsm_subtract_bywalk(a, b);  break;

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

		if (op != OP_EQUAL && q == NULL) {
			perror("fsm_op");
			exit(EXIT_FAILURE);
		}

		r = !r;

		fsm = q;
	}

	/* henceforth, r is $?-convention (0 for success) */

	if (fsm == NULL) {
		return r;
	}

	/* TODO: OP_EQUAL ought to have the same CLI interface as a predicate */
	if (query != NULL) {
		/* XXX: this symbol is used like an enum here. It's a bit of a kludge */
		if (query == query_countstates) {
			printf("%u\n", fsm_countstates(fsm));
			return 0;
		} else {
			r |= !fsm_all(fsm, query);
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

	if (print) {
		fsm_print(fsm, stdout, format);
	}

	fsm_free(fsm);

	return r;
}

