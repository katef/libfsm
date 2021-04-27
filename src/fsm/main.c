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
#include <fsm/parser.h>

#include <adt/stateset.h> /* XXX */

#include "libfsm/internal.h" /* XXX */

#include "wordgen.h"

#if defined(__APPLE__) && defined(__MACH__) && defined(MACOS_HAS_NO_CLOCK_GETITME)
#include <mach/clock.h>
#include <mach/mach.h>

/* we're running Darwin/macOS, so we don't necessarily have clock_gettime,
 * so we do it the ugly way...
 */

#define CLOCK_MONOTONIC 0
static int clock_gettime(int clk_id, struct timespec *ts)
{
	clock_serv_t cclock;
	mach_timespec_t mts;

	(void)clk_id;

	host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);

	ts->tv_sec = mts.tv_sec;
	ts->tv_nsec = mts.tv_nsec;

	return 0;
}
#endif /* defined(__APPLE__) && defined(__MACH__) && !defined(MACOS_HAS_CLOCK_GETITME) */

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
	OP_RM_EPSILONS = (12 << 1) | 1,

	/* binary */
	OP_CONCAT      = ( 7 << 1) | 0,
	OP_UNION       = ( 8 << 1) | 0,
	OP_INTERSECT   = ( 9 << 1) | 0,
	OP_SUBTRACT    = (10 << 1) | 0,
	OP_EQUAL       = (11 << 1) | 0
};

static int
query_countstates(const struct fsm *fsm, fsm_state_t state)
{
	(void) fsm;
	(void) state;

	/* never called */
	abort();
}

static int
query_epsilonclosure(const struct fsm *fsm, fsm_state_t state)
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
	printf("       fsm {-p} [-l <language>] [-aCcEgwX] [-k <io>] [-e <prefix>]\n");
	printf("       fsm {-dmr | -t <transformation>} [-i <iterations>] [<file.fsm> | <file-a> <file-b>]\n");
	printf("       fsm {-q <query>} [<file>]\n");
	printf("       fsm {-W <maxlen>} <file.fsm>\n");
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
		{ "api",   fsm_print_api   },
		{ "awk",   fsm_print_awk   },
		{ "c",     fsm_print_c     },
		{ "dot",   fsm_print_dot   },
		{ "fsm",   fsm_print_fsm   },
		{ "ir",    fsm_print_ir    },
		{ "json",  fsm_print_json  },
		{ "vmc",   fsm_print_vmc   },
		{ "vmdot", fsm_print_vmdot },
		{ "rust",  fsm_print_rust  },
		{ "sh",    fsm_print_sh    },
		{ "go",    fsm_print_go    },

		{ "amd64",      fsm_print_vmasm            },
		{ "amd64_att",  fsm_print_vmasm_amd64_att  },
		{ "amd64_nasm", fsm_print_vmasm_amd64_nasm },
		{ "amd64_go",   fsm_print_vmasm_amd64_go   }
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
		int (*)(const struct fsm *, fsm_state_t))))
(const struct fsm *, fsm_state_t)
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
			int (*)(const struct fsm *, fsm_state_t));
		int (*pred)(const struct fsm *, fsm_state_t);
	} a[] = {
		{ "isdfa",             fsm_all, fsm_isdfa             },
		{ "dfa",               fsm_all, fsm_isdfa             },
		{ "count",             NULL,    query_countstates     },
		{ "epsilonclosure",    NULL,    query_epsilonclosure  },
		{ "iscomplete",        fsm_all, fsm_iscomplete        },
		{ "hasend",            fsm_has, fsm_isend             },
		{ "end",               fsm_has, fsm_isend             },
		{ "accept",            fsm_has, fsm_isend             },
		{ "hasaccept",         fsm_has, fsm_isend             },
		{ "ambiguity",         fsm_has, fsm_hasnondeterminism },
		{ "hasambiguity",      fsm_has, fsm_hasnondeterminism },
		{ "hasnondeterminism", fsm_has, fsm_hasnondeterminism },
		{ "hasepsilons",       fsm_has, fsm_hasepsilons       },
		{ "epsilons",          fsm_has, fsm_hasepsilons       }
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
		{ "glush",       OP_RM_EPSILONS },
		{ "glushovize",  OP_RM_EPSILONS },
		{ "min",         OP_MINIMISE    },
		{ "minimise",    OP_MINIMISE    },
		{ "trim",        OP_TRIM        },
		{ "glushkovise", OP_RM_EPSILONS },
		{ "remove_epsilons", OP_RM_EPSILONS },

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

#if 0
static unsigned int word_maxlen = 16;
static unsigned int num_words = 8;
static uint64_t seed = 0xdfa7231bc;

static void
gen_words(FILE *f, const struct fsm *fsm)
{
	struct dfa_wordgen_params params = {
		.minlen = 1,
		.maxlen = word_maxlen,

		.prob_stop  = 0.3f,
		.eps_weight = 0.0f,
	};

	struct prng_state prng;

	memset(&prng, 0, sizeof prng);
	prng_seed(&prng, seed);

	fsm_generate_words_to_file(fsm, &params, &prng, num_words, f);
}
#endif /* 0 */

static struct fsm *fsm_to_cleanup = NULL;

static void
do_fsm_cleanup(void)
{
	if (fsm_to_cleanup != NULL) {
		fsm_free(fsm_to_cleanup);
		fsm_to_cleanup = NULL;
	}
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

	int (*query)(const struct fsm *, fsm_state_t);
	int (*walk )(const struct fsm *,
		 int (*)(const struct fsm *, fsm_state_t));

	atexit(do_fsm_cleanup);

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

		while (c = getopt(argc, argv, "h" "aCcgwXEe:k:i:" "xpq:l:dGmrt:W:"), c != -1) {
			switch (c) {
			case 'a': opt.anonymous_states  = 1;          break;
			case 'c': opt.consolidate_edges = 1;          break;
			case 'C': opt.comments		= 0;          break;
			case 'g': opt.group_edges       = 1;          break;
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
			case 'G': op = op_name("glushkovise");        break;
			case 'E': op = op_name("remove_epsilons");    break;
			case 'W':
				/* print = gen_words; */
				/* num_words = strtoul(optarg, NULL, 10); */
				/* XXX: error handling */
				fprintf(stderr, "not yet implemented.\n");
				exit(EXIT_FAILURE);
				break;

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
		case OP_RM_EPSILONS: r = fsm_remove_epsilons(q);  break;
		case OP_TRIM:        r = fsm_trim(q,
		    FSM_TRIM_START_AND_END_REACHABLE, NULL);
			if (r >= 0) { /* returns number of states removed */
				r = 1;
			}
			break;

		case OP_CONCAT:      q = fsm_concat(a, b, NULL); break;
		case OP_UNION:       q = fsm_union(a, b, NULL); break;
		case OP_INTERSECT:   q = fsm_intersect(a, b); break;
		case OP_SUBTRACT:    q = fsm_subtract(a, b);  break;

		case OP_MINIMISE:
			r = fsm_determinise(q);
			if (r == 1) {
				r = fsm_minimise(q);
			}
			break;

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

		fsm_to_cleanup = q;

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
		} else if (query == query_epsilonclosure) {
			struct state_set **closures;
			fsm_state_t i, j;
			size_t n;
			struct state_iter it;

			closures = epsilon_closure(fsm);
			if (closures == NULL) {
				return -1;
			}

			n = fsm_countstates(fsm);

			for (i = 0; i < n; i++) {
				int first = 1;
				if (!opt.anonymous_states) {
					printf("%u: ", (unsigned) i);
				}
				for (state_set_reset(closures[i], &it); state_set_next(&it, &j); ) {
					printf("%s%u",
						first ? "" : " ",
						(unsigned) j);
					first = 0;
				}
				printf("\n");
			}

			closure_free(closures, fsm->statecount);

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
			fsm_state_t state;
			int e;

			if (xfiles) {
				FILE *f;

				f = xopen(argv[0]);

				e = fsm_exec(fsm, fsm_fgetc, f, &state, NULL);

				fclose(f);
			} else {
				const char *s;

				s = argv[i];

				e = fsm_exec(fsm, fsm_sgetc, &s, &state, NULL);
			}

			if (e != 1) {
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
	fsm_to_cleanup = NULL;

	return r;
}

