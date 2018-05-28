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

enum mode {
    MODE_DETERMINISE,
    MODE_MINIMISE,
    MODE_REVERSE,
    MODE_TRIM,
    MODE_COMPLEMENT,

    MODE_MATCH,
};

struct config {
    enum mode mode;
    enum re_dialect dialect;
    int verbosity;
    unsigned iterations;
    bool keep_nfa;
    const char *re;
    const char *input;
};

#define bench_libfsm_VERSION_MAJOR 0
#define bench_libfsm_VERSION_MINOR 1
#define bench_libfsm_VERSION_PATCH 0
#define bench_libfsm_AUTHOR "Scott Vokes"

static void
usage(const char *msg)
{
	if (msg != NULL) {
		fprintf(stderr, "%s\n\n", msg);
	}

	fprintf(stderr, "bench_libfsm v. %d.%d.%d by %s\n",
		bench_libfsm_VERSION_MAJOR, bench_libfsm_VERSION_MINOR,
		bench_libfsm_VERSION_PATCH, bench_libfsm_AUTHOR);
	fprintf(stderr,
		"Usage: bench_libfsm [-h] [-d <dialect>] [-i <iterations>]\n"
		"    [-m <mode>] [-n] <regex> [<input>]\n"
		"\n"
		"-h: Print help.\n"
		"-d <dialect> can be one of: like literal glob native pcre sql\n"
		"-m <mode> can be one of: 'd'eterminise, 'm'inimize, 'r'everse\n"
		"-n: When matching input argument, do not convert NFA to DFA.\n"
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

static bool
mode_of_optarg(const char *optarg, enum mode *m)
{
	switch (optarg[0]) {
	case 'c': *m = MODE_COMPLEMENT;  break;
	case 'd': *m = MODE_DETERMINISE; break;
	case 'm': *m = MODE_MINIMISE;    break;
	case 'r': *m = MODE_REVERSE;     break;
	case 't': *m = MODE_TRIM;        break;

	default:
		return false;
	}

	return true;
}

static const char *
mode_name(enum mode m)
{
	switch (m) {
	case MODE_DETERMINISE: return "determinise";
	case MODE_MINIMISE:    return "minimise";
	case MODE_REVERSE:     return "reverse";
	case MODE_MATCH:       return "match";
	case MODE_TRIM:        return "trim";
	case MODE_COMPLEMENT:  return "complement";

	default:
		return "?";
	}
}

struct scanner {
	char tag;
	void *magic;
	const char *str;
	size_t size;
	size_t offset;
};

static int
scanner_next(void *opaque)
{
	struct scanner *s;
	char c;

	s = opaque;
	assert(s->tag == 'S');

	if (s->offset == s->size) {
		return EOF;
	}

	c = s->str[s->offset];
	s->offset++;

	return (unsigned char) c;
}

static const struct fsm_options re_options = {
	.anonymous_states  = 1,
	.consolidate_edges = 1,
	.always_hex        = 1
};

static struct fsm *
build_fsm(enum re_dialect dialect, const char *re, struct re_err *err)
{
	struct fsm *fsm = NULL;

	struct scanner s = {
		.tag    = 'S',
		.magic  = &s.magic,
		.str    = re,
		.size   = strlen(re),
		.offset = 0
	};

	assert(err != NULL);

	fsm = re_comp(dialect, scanner_next, &s, &re_options, RE_MULTI, err);

	assert(s.magic == &s.magic);

	return fsm;
}

static struct fsm_state *
exec_fsm(struct fsm *f, const char *input, size_t len)
{
	struct fsm_state *state = NULL;

	struct scanner s = {
		.tag    = 'S',
		.magic  = &s.magic,
		.str    = input,
		.size   = len,
		.offset = 0
	};

	state = fsm_exec(f, scanner_next, &s);

	assert(s.str == input);
	assert(s.magic == &s.magic);

	return state;
}

static int
run(struct config *cfg)
{
	int res = 0;
	struct fsm *f = NULL;
	double elapsed = 0.0;
	const size_t len = (cfg->input ? strlen(cfg->input) : 0);
	unsigned iter;

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

	printf("%s [%u]: %s", mode_name(cfg->mode), cfg->iterations,
		cfg->iterations == 1 ? "" : "[ ");

	for (iter = 0; iter < cfg->iterations; iter++) {
		/* build a FSM from a regex */
		struct re_err err;
		struct timeval pre, post;

		f = build_fsm(cfg->dialect, cfg->re, &err);
		if (f == NULL) {
			re_perror(cfg->dialect, &err, NULL, cfg->re);
			res = EXIT_FAILURE;
			goto cleanup;
		}

		if (cfg->mode == MODE_MATCH && !cfg->keep_nfa) {
			if (!fsm_determinise(f)) {
				fprintf(stderr, "FAIL: fsm_determinise\n");
				res = EXIT_FAILURE;
				goto cleanup;
			}
		}

		/* run the operation under test, saving the time */
		if (-1 == gettimeofday(&pre, NULL)) {
			assert(false);
		}

		switch (cfg->mode) {
		case MODE_DETERMINISE:
			if (!fsm_determinise(f)) {
				fprintf(stderr, "FAIL: fsm_determinise\n");
				res = EXIT_FAILURE;
				goto cleanup;
			}
			break;

		case MODE_MINIMISE:
			if (!fsm_minimise(f)) {
				fprintf(stderr, "FAIL: fsm_minimise\n");
				res = EXIT_FAILURE;
				goto cleanup;
			}
			break;

		case MODE_REVERSE:
			if (!fsm_reverse(f)) {
				fprintf(stderr, "FAIL: fsm_reverse\n");
				res = EXIT_FAILURE;
				goto cleanup;
			}
			break;

		case MODE_TRIM:
			if (!fsm_trim(f)) {
				fprintf(stderr, "FAIL: fsm_trim\n");
				res = EXIT_FAILURE;
				goto cleanup;
			}
			break;

		case MODE_COMPLEMENT:
			if (!fsm_complement(f)) {
				fprintf(stderr, "FAIL: fsm_complement\n");
				res = EXIT_FAILURE;
				goto cleanup;
			}
			break;

		case MODE_MATCH: {
			struct fsm_state *state = exec_fsm(f, cfg->input, len);
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

		fsm_free(f);
		f = NULL;
	}

	if (cfg->iterations > 1) {
		printf("] ");
	}

	/*
	 * TODO: Would it be worth doing stddev or other calculations here?
	 * The formatting should make it easy to do with awk/etc. otherwise.
	 */
	printf("=> total %g msec (avg %g msec)\n", elapsed, elapsed / cfg->iterations);

cleanup:

	if (f != NULL) {
		fsm_free(f);
	}

	return res;
}

int
main(int argc, char *argv[])
{
	struct config cfg = {
		.mode = MODE_DETERMINISE,
		.dialect = RE_PCRE,
		.iterations = 1,
		.keep_nfa = false,
	};

	{
		int c;

		while (c = getopt(argc, argv, "hd:i:m:nv"), c != -1) {
			switch (c) {
			case 'd':
				cfg.dialect = dialect_name(optarg);
				break;

			case 'i':
				cfg.iterations = strtoul(optarg, NULL, 10);
				if (cfg.iterations == 0) {
					usage("Invalid iterations");
				}
				break;

			case 'm':
				if (!mode_of_optarg(optarg, &cfg.mode)) {
					usage("Invalid mode");
				}
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
		cfg.mode = MODE_MATCH;
	}

	if (cfg.re == NULL) {
		usage("Missing required argument: regex");
	}

	return run(&cfg);
}

