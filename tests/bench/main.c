/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <sys/time.h>

#include <unistd.h>
#include <getopt.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

#include <fsm/bool.h>
#include <fsm/cost.h>
#include <fsm/fsm.h>
#include <fsm/out.h>
#include <fsm/options.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <re/re.h>

enum op {
	OP_COMPLETE,

	OP_COMPLEMENT,
	OP_DETERMINISE,
	OP_REVERSE,
	OP_MINIMISE,
	OP_TRIM,

	OP_MATCH,
};

struct config {
	enum op op;
	enum re_dialect dialect;
	int verbosity;
	unsigned iterations;
	bool keep_nfa;
	const char *re;
	const char *input;
};

static void
usage(const char *msg)
{
	if (msg != NULL) {
		fprintf(stderr, "%s\n\n", msg);
	}

	fprintf(stderr, "usage: bench_libfsm [-r <dialect>] [-i <iterations>]\n"
	                "                    [-t <transformation>] [-n] <regex> [<text>]\n");
	fprintf(stderr, "       bench_libfsm [-h]\n");

	fprintf(stderr, "\n"
	                "  -h: Print help.\n"
	                "  -r <dialect> can be one of: like literal glob native pcre sql\n"
	                "  -t <transformation> can be one of: determinise, minimize, reverse\n"
	                "  -n: When matching input argument, do not convert NFA to DFA.\n"
	                );
}

static enum re_dialect
dialect_name(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum re_dialect dialect;
	} a[] = {
/* TODO:
		{ "ere",     RE_ERE     },
		{ "bre",     RE_BRE     },
		{ "plan9",   RE_PLAN9   },
		{ "js",      RE_JS      },
		{ "python",  RE_PYTHON  },
		{ "sql99",   RE_SQL99   },
*/
		{ "like",    RE_LIKE    },
		{ "literal", RE_LITERAL },
		{ "glob",    RE_GLOB    },
		{ "native",  RE_NATIVE  },
		{ "pcre",    RE_PCRE    },
		{ "sql",     RE_SQL     }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].dialect;
		}
	}

	fprintf(stderr, "unrecognised regexp dialect \"%s\"; valid dialects are: ", name);

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
		{ "trim",        OP_TRIM        }
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

static int
run(struct config *cfg)
{
	int res = 0;
	struct fsm *fsm = NULL;
	double elapsed = 0.0;
	const size_t len = (cfg->input ? strlen(cfg->input) : 0);
	unsigned i;

	static const struct fsm_options opt = {
		.anonymous_states  = 1,
		.consolidate_edges = 1,
		.always_hex        = 1
	};

	if (cfg->verbosity > 0) {
		if (cfg->iterations != 1) {
			printf("iterations: %u, ", cfg->iterations);
		}
		printf("regex: '%s'[%zu]", cfg->re, strlen(cfg->re));
		if (cfg->input != NULL) {
			printf(", input: '%s'[%zu]", cfg->input, strlen(cfg->input));
		}
		printf("\n");
	}

	for (i = 0; i < cfg->iterations; i++) {
		struct re_err err;
		struct timeval pre, post;
		const char *s;

		s = cfg->re;

		fsm = re_comp(cfg->dialect, fsm_sgetc, &s, &opt, RE_MULTI, &err);
		if (fsm == NULL) {
			re_perror(cfg->dialect, &err, NULL, cfg->re);
			res = EXIT_FAILURE;
			goto cleanup;
		}

		if (cfg->op == OP_MATCH && !cfg->keep_nfa) {
			if (!fsm_determinise(fsm)) {
				fprintf(stderr, "FAIL: fsm_determinise\n");
				res = EXIT_FAILURE;
				goto cleanup;
			}
		}

		/* run the operation under test, saving the time */
		if (-1 == gettimeofday(&pre, NULL)) {
			assert(false);
		}

		switch (cfg->op) {
		case OP_COMPLETE:
			if (!fsm_complete(fsm, fsm_isany)) {
				fprintf(stderr, "FAIL: fsm_complete\n");
				res = EXIT_FAILURE;
				goto cleanup;
			}
			break;

		case OP_COMPLEMENT:
			if (!fsm_complement(fsm)) {
				fprintf(stderr, "FAIL: fsm_complement\n");
				res = EXIT_FAILURE;
				goto cleanup;
			}
			break;

		case OP_REVERSE:
			if (!fsm_reverse(fsm)) {
				fprintf(stderr, "FAIL: fsm_reverse\n");
				res = EXIT_FAILURE;
				goto cleanup;
			}
			break;

		case OP_DETERMINISE:
			if (!fsm_determinise(fsm)) {
				fprintf(stderr, "FAIL: fsm_determinise\n");
				res = EXIT_FAILURE;
				goto cleanup;
			}
			break;

		case OP_MINIMISE:
			if (!fsm_minimise(fsm)) {
				fprintf(stderr, "FAIL: fsm_minimise\n");
				res = EXIT_FAILURE;
				goto cleanup;
			}
			break;

		case OP_TRIM:
			if (!fsm_trim(fsm)) {
				fprintf(stderr, "FAIL: fsm_trim\n");
				res = EXIT_FAILURE;
				goto cleanup;
			}
			break;

		case OP_MATCH: {
			const char *s;
			struct fsm_state *state;

			s = cfg->input;

			state = fsm_exec(fsm, fsm_sgetc, &s);

			(void) state; /* may not match */
			break;
		}

		default:
			fprintf(stderr, "(MATCH FAIL)\n");
			assert(false);
		}

		if (-1 == gettimeofday(&post, NULL)) {
			assert(false);
		}

		double msec = 1000.0 * (post.tv_sec - pre.tv_sec) +
			(post.tv_usec / 1000.0 - pre.tv_usec / 1000.0);
		elapsed += msec;
		if (cfg->iterations > 1) {
			printf("%g ", msec);
		}

		fsm_free(fsm);
		fsm = NULL;
	}

	/*
	 * TODO: Would it be worth doing stddev or other calculations here?
	 * The formatting should make it easy to do with awk/etc. otherwise.
	 */
	printf("=> total %g msec (avg %g msec)\n", elapsed, elapsed / cfg->iterations);

cleanup:

	if (fsm != NULL) {
		fsm_free(fsm);
	}

	return res;
}

int
main(int argc, char *argv[])
{
	struct config cfg = {
		.op         = OP_DETERMINISE,
		.dialect    = RE_PCRE,
		.iterations = 1,
		.keep_nfa   = false,
	};

	{
		int c;

		while (c = getopt(argc, argv, "hr:i:t:nv"), c != -1) {
			switch (c) {
			case 'r':
				cfg.dialect = dialect_name(optarg);
				break;

			case 'i':
				cfg.iterations = strtoul(optarg, NULL, 10);
				if (cfg.iterations == 0) {
					usage("Invalid iterations");
				}
				break;

			case 't':
				cfg.op = op_name(optarg);
				break;

			case 'n':
				cfg.keep_nfa = true;
				break;

			case 'v':
				cfg.verbosity++;
				break;

			case 'h':
				usage(NULL);
				return EXIT_SUCCESS;

			case '?':
			default:
				usage(NULL);
				return EXIT_FAILURE;
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (argc > 0) {
		cfg.re = argv[0];
	}
	if (argc > 1) {
		cfg.input = argv[1];
		cfg.op    = OP_MATCH;
	}

	if (cfg.re == NULL) {
		usage("Missing required argument: regex");
		exit(EXIT_FAILURE);
	}

	return run(&cfg);
}

