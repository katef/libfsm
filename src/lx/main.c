/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 2

#include "lx.h"

#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/cost.h>
#include <fsm/options.h>

#include <re/re.h>

#include "parser.h"

#include "libfsm/internal.h" /* XXX */

#include "print.h"
#include "ast.h"
#include "tokens.h"

enum api_tokbuf  api_tokbuf;
enum api_getc    api_getc;
enum api_exclude api_exclude;

struct prefix prefix = {
	"lx_",
	"TOK_",
	""
};

struct fsm_options opt;

int print_progress;
int keep_nfa;

struct ast_zone *cur_zone = NULL;
unsigned zn = 0;
int zerror = 0;
static pthread_mutex_t zmtx = PTHREAD_MUTEX_INITIALIZER;

void
lx_mutex_lock(void)
{
	pthread_mutex_lock(&zmtx);
}

void
lx_mutex_unlock(void)
{
	pthread_mutex_unlock(&zmtx);
}


static
void usage(void)
{
	printf("usage: lx [-nQX] [-C <concurrency>] [-b <tokbuf>] [-g <getc>] [-l <language>] [-et <prefix>] [-k <io>] [-x <exclude>]\n");
	printf("       lx -h\n");
}

/*
 * True if a zone number is important enough to print, just to cut down
 * the amount of output to prevent it from flooding the terminal.
 * This is the sequence n * 10^x where n < 10
 */
int
important(unsigned int n)
{
	for (;;) {
		if (n < 10) {
			return 1;
		}

		if ((n % 10) != 0) {
			return 0;
		}

		n /= 10;
	}
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

static lx_print *
print_name(const char *name)
{
	size_t i;

	struct {
		const char *name;
		lx_print *f;
	} a[] = {
		{ "test", NULL          },
		{ "dot",  lx_print_dot  },
		{ "dump", lx_print_dump },
		{ "zdot", lx_print_zdot },
		{ "c",    lx_print_c    },
		{ "h",    lx_print_h    }
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

static enum api_tokbuf
lang_tokbuf(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum api_tokbuf tokbuf;
	} a[] = {
		{ "dyn",   API_DYNBUF   },
		{ "fixed", API_FIXEDBUF }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].tokbuf;
		}
	}

	fprintf(stderr, "unrecognised token buffer scheme; valid schemes are: ");

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			i + 1 < sizeof a / sizeof *a ? ", " : "\n");
	}

	exit(EXIT_FAILURE);
}

static enum api_getc
lang_getc(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum api_getc getc;
	} a[] = {
		{ "fgetc",  API_FGETC  },
		{ "sgetc",  API_SGETC  },
		{ "agetc",  API_AGETC  },
		{ "fdgetc", API_FDGETC }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].getc;
		}
	}

	fprintf(stderr, "unrecognised getc scheme; valid schemes are: ");

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			i + 1 < sizeof a / sizeof *a ? ", " : "\n");
	}

	exit(EXIT_FAILURE);
}

static enum api_exclude
lang_exclude(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum api_exclude item;
	} a[] = {
		{ "name",     API_NAME     },
		{ "example",  API_EXAMPLE  },
		{ "comments", API_COMMENTS },
		{ "pos",      API_POS      },
		{ "buf",      API_BUF      }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].item;
		}
	}

	fprintf(stderr, "unrecognised item to exclude; valid items are: ");

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		fprintf(stderr, "%s%s",
			a[i].name,
			i + 1 < sizeof a / sizeof *a ? ", " : "\n");
	}

	exit(EXIT_FAILURE);
}

static int
zone_equal(const struct ast_zone *a, const struct ast_zone *b)
{
	struct fsm *x, *y;
	struct fsm *q;
	int r;

	assert(a != NULL);
	assert(b != NULL);

	if (!tok_equal(a->fsm, b->fsm)) {
		return 0;
	}

	r = fsm_equal(a->fsm, b->fsm);
	if (r == -1) {
		return -1;
	}

	if (!r) {
		return 0;
	}

	x = fsm_clone(a->fsm);
	if (x == NULL) {
		return -1;
	}

	y = fsm_clone(b->fsm);
	if (y == NULL) {
		fsm_free(y);
		return -1;
	}

	q = fsm_union(x, y, NULL);
	if (q == NULL) {
		fsm_free(x);
		fsm_free(y);
		return -1;
	}

	{
		/* opt.carryopaque = carryopaque; */

		if (!fsm_determinise(q)) {
			fsm_free(q);
			return -1;
		}

		/* opt.carryopaque = NULL; */
	}

	{
		const struct ast_mapping *m;
		fsm_state_t i;

		for (i = 0; i < q->statecount; i++) {
			if (!fsm_isend(q, i)) {
				continue;
			}

			m = ast_getendmapping(q, i);

			if (LOG()) {
				fprintf(stderr, "zone_equal: asserting ast_getendmapping(q, state %d) != NULL: %p\n", i, (void *)m);
			}
			assert(m != NULL);

			if (m->conflict != NULL) {
				/* TODO: free conflict set */
				fsm_free(q);
				return 0;
			}
		}
	}

	return 1;
}

void *
zone_minimise(void *arg)
{
	(void) arg;

	for (;;) {
		struct ast_zone    *z;
		struct ast_mapping *m;
		fsm_state_t start;

		pthread_mutex_lock(&zmtx);
		{
			z = cur_zone;
			if (z == NULL || zerror != 0) {
				pthread_mutex_unlock(&zmtx);
				return NULL;
			}
			cur_zone = cur_zone->next;

			assert(z->fsm == NULL);
			if (print_progress) {
				if (important(zn)) {
					fprintf(stderr, " z%u", zn);
				}
				zn++;
			}
		}
		pthread_mutex_unlock(&zmtx);

		z->fsm = fsm_new(&opt);
		if (z->fsm == NULL) {
			pthread_mutex_lock(&zmtx);
			zerror = errno;
			pthread_mutex_unlock(&zmtx);
			return "fsm_new";
		}

		if (!fsm_addstate(z->fsm, &start)) {
			pthread_mutex_lock(&zmtx);
			zerror = errno;
			pthread_mutex_unlock(&zmtx);
			return "fsm_addstate";
		}

		for (m = z->ml; m != NULL; m = m->next) {
			fsm_state_t ms;
			struct fsm_combine_info combine_info;

			assert(m->fsm != NULL);

			if (!keep_nfa) {
				if (!fsm_determinise(m->fsm)) {
					pthread_mutex_lock(&zmtx);
					zerror = errno;
					pthread_mutex_unlock(&zmtx);
					return "fsm_minimise";
				}
				if (!fsm_minimise(m->fsm)) {
					pthread_mutex_lock(&zmtx);
					zerror = errno;
					pthread_mutex_unlock(&zmtx);
					return "fsm_minimise";
				}
			}

			/* Attach this mapping to each end state for this FSM */
			if (LOG()) {
				fprintf(stderr, "zone_minimise: ast_setendmapping(m->fsm, m: %p)\n",
				    (void *)m);
			}

			if (!ast_setendmapping(m->fsm, m)) {
				assert(!"failed");
			}

			(void) fsm_getstart(m->fsm, &ms);

			z->fsm = fsm_merge(z->fsm, m->fsm, &combine_info);
			if (z->fsm == NULL) {
				pthread_mutex_lock(&zmtx);
				zerror = errno;
				pthread_mutex_unlock(&zmtx);
				return "fsm_union";
			}

#ifndef NDEBUG
			m->fsm = NULL;
#endif

			ms    += combine_info.base_b;
			start += combine_info.base_a;

			if (!fsm_addedge_epsilon(z->fsm, start, ms)) {
				pthread_mutex_lock(&zmtx);
				zerror = errno;
				pthread_mutex_unlock(&zmtx);
				return "fsm_addedge_epsilon";
			}
		}

		fsm_setstart(z->fsm, start);
	}
}

void *
zone_determinise(void *arg)
{
	(void) arg;

	for (;;) {
		struct ast_zone *z;

		pthread_mutex_lock(&zmtx);
		{
			z = cur_zone;
			if (z == NULL || zerror != 0) {
				pthread_mutex_unlock(&zmtx);
				return NULL;
			}
			cur_zone = cur_zone->next;

			if (print_progress) {
				if (important(zn)) {
					fprintf(stderr, " z%u", zn);
				}
				zn++;
			}
		}
		pthread_mutex_unlock(&zmtx);

		if (!fsm_determinise(z->fsm)) {
			pthread_mutex_lock(&zmtx);
			zerror = errno;
			pthread_mutex_unlock(&zmtx);
			return "fsm_determinise";
		}
	}
}

int
run_threads(int concurrency, void *(*fn)(void *))
{
	pthread_t *tds;
	int i;

	assert(fn != NULL);

	tds = malloc(concurrency * sizeof *tds);
	if (tds == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}

	for (i = 0; i < concurrency; i++) {
		int r;

		r = pthread_create(&tds[i], NULL, fn, NULL);
		if (r != 0) {
			perror("pthread_create");
			return EXIT_FAILURE;
		}
	}

	for (i = 0; i < concurrency; i++) {
		void *rv;
		int r;

		r = pthread_join(tds[i], &rv);
		if (r != 0) {
			perror("pthread_join");
			return EXIT_FAILURE;
		}

		if (rv != NULL) {
			char *f = rv;
			errno = zerror;
			perror(f);
			return EXIT_FAILURE;
		}
	}

	free(tds);

	return 0;
}

int
main(int argc, char *argv[])
{
	struct ast *ast;
	lx_print *print;
	int concurrency;

	print = lx_print_c;
	keep_nfa = 0;
	print_progress = 0;
	concurrency = 1;

	/* TODO: populate options */
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;
	opt.comments          = 1;
	opt.io                = FSM_IO_GETC;
	opt.prefix            = NULL;

	{
		int c;

		while (c = getopt(argc, argv, "h" "C:Xe:t:k:" "vb:g:l:nQx:"), c != -1) {
			switch (c) {
			case 'C':
				concurrency = atoi(optarg);
				break;

			case 'X':
				opt.always_hex = 1;
				break;

			case 'e':
				prefix.api = optarg;
				break;

			case 't':
				prefix.tok = optarg;
				break;

			case 'k':
				opt.io = io(optarg);
				break;

			case 'b': api_tokbuf  |= lang_tokbuf(optarg);  break;
			case 'g': api_getc    |= lang_getc(optarg);    break;
			case 'x': api_exclude |= lang_exclude(optarg); break;

			case 'l':
				print = print_name(optarg);
				break;

			case 'n':
				keep_nfa = 1;
				break;

			case 'Q':
				print_progress = 1;
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

		if (argc > 0) {
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (keep_nfa && print != lx_print_dot) {
		fprintf(stderr, "-n is for .dot output only\n");
		return EXIT_FAILURE;
	}

	if (api_tokbuf && (print != lx_print_c && print != lx_print_h && print != lx_print_dump)) {
		fprintf(stderr, "-b is for .c/.h output only\n");
		return EXIT_FAILURE;
	}

	if (api_getc && (print != lx_print_c && print != lx_print_h && print != lx_print_dump)) {
		fprintf(stderr, "-g is for .c/.h output only\n");
		return EXIT_FAILURE;
	}

	if (api_getc && opt.io != FSM_IO_GETC) {
		fprintf(stderr, "-g is for -k getc output only\n");
		return EXIT_FAILURE;
	}

	if (0 != strcmp(prefix.api, "lx_")) {
		prefix.lx = prefix.api;
	}

	if ((api_exclude & API_COMMENTS) != 0) {
		opt.comments = 0;
	}

	{
		if (print_progress) {
			fprintf(stderr, "-- parsing:");
		}

		ast = lx_parse(stdin, &opt);
		if (ast == NULL) {
			return EXIT_FAILURE;
		}

		if (print_progress) {
			fprintf(stderr, "\n");
		}
	}

	/*
	 * What happens here is a transformation of the AST. This is equivalent to
	 * compiling to produce an IR, however the IR in lx's case is practically
	 * identical to the AST, so the same data structure is kept for simplicity.
	 *
	 * The important steps are:
	 *  1. Set FSM end state to point to each ast_mapping;
	 *  2. Minimize each mapping's regex;
	 *  3. Union all regexps within one mapping to produce one regexp per zone;
	 *  4. NFA->DFA on each zone's regexp.
	 *
	 * This produces as minimal state machines as possible, without losing track
	 * of which end state is associated with each mapping. In other words,
	 * without losing track of which regexp maps to which token.
	 */
	if (print != lx_print_h) {
		if (print_progress) {
			fprintf(stderr, "-- minimise AST:");
			zn = 0;
		}

		cur_zone = ast->zl;
		if (run_threads(concurrency, zone_minimise)) {
			return EXIT_FAILURE;
		}

		if (print_progress) {
			fprintf(stderr, "\n");
		}

		if (!keep_nfa) {
			if (print_progress) {
				fprintf(stderr, "-- determinise AST:");
				zn = 0;
			}

			/* opt.carryopaque = carryopaque; */
			cur_zone = ast->zl;
			if (run_threads(concurrency, zone_determinise)) {
				return EXIT_FAILURE;
			}
			/* opt.carryopaque = NULL; */

			if (print_progress) {
				fprintf(stderr, "\n");
			}
		}
	}

	/*
	 * De-duplicate equivalent zones.
	 * This converts the tree of zones to a DAG.
	 * TODO: Fix
	 */
	if (0 && print != lx_print_h) {
		struct ast_zone *z, **p;
		int changed;
		unsigned int zn, zd, zp;

		if (print_progress) {
			zp = 1;
		}

		do {
			if (print_progress) {
				fprintf(stderr, "-- de-duplicate zones, pass %u:", zp++);
				zn = 0;
				zd = 0;
			}

			changed = 0;

			for (z = ast->zl; z != NULL; z = z->next) {
				if (print_progress) {
					if (important(zn)) {
						fprintf(stderr, " z%u", zn);
					}
					zn++;
				}

				for (p = &z->next; *p != NULL; p = &(*p)->next) {
					struct ast_zone *q;
					fsm_state_t i;
					int r;

					r = zone_equal(z, *p);
					if (r == -1) {
						perror("zone_equal");
						exit(EXIT_FAILURE);
					}

					if (!r) {
						continue;
					}

					for (q = ast->zl; q != NULL; q = q->next) {
						for (i = 0; i < q->fsm->statecount; i++) {
							struct ast_mapping *m;

							if (!fsm_isend(q->fsm, i)) {
								continue;
							}

							m = ast_getendmapping(q->fsm, i);
							if (LOG()) {
								fprintf(stderr, "main: m <- ast_getendmapping(dst_fsm, i]: %d) = %p    // remove duplicate zones?\n", i, (void *)m);
							}
							assert(m != NULL);

							if (m->to == *p) {
								m->to = z;
								/* TODO: this mapping is now possibly a duplicte of another; if so, remove it */
							}
						}
					}

					{
						struct ast_zone *dead;

						dead = *p;

						*p = dead->next;

						dead->next = NULL;
						fsm_free(dead->fsm);
						/* TODO: free dead->ml */
						/* TODO: free dead->vl? */
						free(dead);
					}

					changed = 1;

					if (print_progress) {
						fprintf(stderr, "*");
						zd++;
					}
				}
			}

			if (print_progress) {
				fprintf(stderr, " (%u deleted)\n", zd);
			}
		} while (changed);
	}

	/*
	 * Semantic checks.
	 */
	if (print != lx_print_h) {
		struct ast_zone  *z;
		unsigned int zn;
		fsm_state_t i;
		int e;

		if (print_progress) {
			fprintf(stderr, "-- semantic checks:");
			zn = 0;
		}

		assert(ast->zl != NULL);

		/* TODO: check for: no end states (same as no tokens?) */
		/* TODO: check for reserved token names (ERROR, EOF etc) */
		/* TODO: don't forget to indicate zone in error messages */
		/* TODO: check for /abc/ -> $a; /abc/ -> $a; */
		/* TODO: check for /abc+/ -> $a; /abc/ -> $a; */

		e = 0;

		for (z = ast->zl; z != NULL; z = z->next) {
			fsm_state_t start;

			assert(z->fsm != NULL);
			assert(z->ml  != NULL);

			if (print_progress) {
				if (important(zn)) {
					fprintf(stderr, " z%u", zn);
				}
				zn++;
			}

			(void) fsm_getstart(z->fsm, &start);

			if (fsm_isend(z->fsm, start)) {
				fprintf(stderr, "start state accepts\n"); /* TODO */
				e = 1;
			}

			/* pick up conflicts flagged by carryopaque() */
			for (i = 0; i < z->fsm->statecount; i++) {
				struct ast_mapping *m;

				if (!fsm_isend(z->fsm, i)) {
					continue;
				}

				m = ast_getendmapping(z->fsm, i);
				if (LOG()) {
					fprintf(stderr, "main: m <- ast_getendmapping(dst_fsm, i]: %d) = %p    // pick up conflicts\n", i, (void *)m);
				}
				assert(m != NULL);

				if (m->conflict != NULL) {
					struct mapping_set *p;
					char buf[50]; /* 50 looks reasonable for an on-screen limit */
					int n;

					n = fsm_example(z->fsm, i, buf, sizeof buf);
					if (-1 == n) {
						perror("fsm_example");
						return EXIT_FAILURE;
					}

					/*
					 * When n == 0, we have two patterns which match the empty string.
					 * Here we defer to the error about the start state accepting,
					 * and it seems redundant to also show an error about both patterns
					 * matching the same input, even if there's a non-empty part.
					 */
					if (n > 0) {
						fprintf(stderr, "ambiguous mappings to ");

						for (p = m->conflict; p != NULL; p = p->next) {
							if (p->m->token != NULL) {
								fprintf(stderr, "$%s", p->m->token->s);
							} else if (p->m->to == NULL) {
								fprintf(stderr, "skip");
							}
							if (p->m->token != NULL && p->m->to != NULL) {
								fprintf(stderr, "/");
							}
							if (p->m->to == ast->global) {
								fprintf(stderr, "global zone");
							} else if (p->m->to != NULL) {
								fprintf(stderr, "z%p", (void *) p->m->to); /* TODO: zindexof(n->to) */
							}

							if (p->next != NULL) {
								fprintf(stderr, ", ");
							}
						}

						/* TODO: escape hex etc */
						fprintf(stderr, "; for example on input '%s%s'\n", buf,
							n >= (int) sizeof buf - 1 ? "..." : "");

						e = 1;
					}
				}
			}
		}

		if (print_progress) {
			fprintf(stderr, "\n");
		}

		if (e && print != lx_print_dot) {
			return EXIT_FAILURE;
		}
	}

	/* XXX: can do this before semantic checks */
	/* TODO: free ast */
	/* TODO: free DFA ast_mappings, created in carryopaque, iff making a DFA. i.e. those which have non-NULL conflict sets */
	if (print == lx_print_h) {
		/* TODO: special case to avoid overhead; free non-minimized NFA */
	}
	if (!keep_nfa && print != lx_print_h) {
		struct ast_zone *z;
		struct ast_mapping *m;
		fsm_state_t i;

		for (z = ast->zl; z != NULL; z = z->next) {
			for (i = 0; i < z->fsm->statecount; i++) {
				if (!fsm_isend(z->fsm, i)) {
					continue;
				}

				m = ast_getendmapping(z->fsm, i);
				if (LOG()) {
					fprintf(stderr, "main: m <- ast_getendmapping(dst_fsm, i]: %d) = %p     // free conflicts\n", i, (void *)m);
				}
				if (m != NULL) {
					assert(m->fsm == NULL);

					/* TODO: free m->conflict, allocated in carryopaque */
					(void) m->conflict;
				}
			}
		}
	}

	{
		if (print_progress) {
			fprintf(stderr, "-- output:");
		}

		if (print != NULL) {
			print(stdout, ast);
		}

		if (print_progress) {
			fprintf(stderr, "\n");
		}
	}

	return EXIT_SUCCESS;
}

/* XXX: we're not interested in leaks in lx for the moment; ASAN is applied
 * for the libraries only. cleanup for lx's own structures should be addressed
 * at some point; excluding this is sloppy, but it's less important right now. */
const char * __asan_default_options(void) { return "detect_leaks=0"; }
