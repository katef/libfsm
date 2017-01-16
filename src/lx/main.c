/* $Id$ */

#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/cost.h>

#include <re/re.h>

#include "parser.h"

#include "libfsm/internal.h" /* XXX */

#include "out/out.h"

#include "ast.h"

enum api_tokbuf  api_tokbuf;
enum api_getc    api_getc;
enum api_exclude api_exclude;

struct prefix prefix = {
	"lx_",
	"TOK_",
	""
};

static
void usage(void)
{
	printf("usage: lx [-n] [-b <tokbuf>] [-g <getc>] [-l <language>] [-et <prefix>] [-x <exclude>]\n");
	printf("       lx -h\n");
}

static void
(*language(const char *name))
(const struct ast *ast, FILE *f)
{
	size_t i;

	struct {
		const char *name;
		void (*out)(const struct ast *ast, FILE *f);
	} a[] = {
		{ "test", NULL       },
		{ "dot",  lx_out_dot },
		{ "c",    lx_out_c   },
		{ "h",    lx_out_h   }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].out;
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
		{ "pos",      API_POS      }
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

static void
carryopaque(struct state_set *set, struct fsm *fsm, struct fsm_state *state)
{
	struct mapping_set *conflict;
	struct ast_mapping *m;
	struct state_set *s;

	assert(set != NULL); /* TODO: right? */
	assert(fsm != NULL);
	assert(state != NULL);
	assert(state->opaque == NULL);

	if (!fsm_isend(fsm, state)) {
		return;
	}

	/*
	 * Here we mark newly-created DFA states with the same AST mapping
	 * as from their corresponding source NFA states. These are the mappings
	 * which indicate which lexical token (and zone transition) is produced
	 * from each accepting state in a particular regexp.
	 *
	 * Because all the accepting states are reachable together, they
	 * should all share the same mapping. So we nominate one to use for the
	 * opaque value and check all other accepting states are the same.
	 */

	for (s = set; s != NULL; s = s->next) {
		if (fsm_isend(fsm, s->state)) {
			m = s->state->opaque;
			break;
		}
	}

	assert(m != NULL);

	conflict = NULL;

	for (s = set; s != NULL; s = s->next) {
		struct ast_mapping *p;

		if (!fsm_isend(fsm, s->state)) {
			continue;
		}

		assert(s->state->opaque != NULL);

		p = s->state->opaque;

		if (m->to == p->to && m->token == p->token) {
			continue;
		}

		if (!ast_addconflict(&conflict, p)) {
			perror("ast_addconflict");
			goto error;
		}
	}

	/* if anything conflicts with m, then m is part of the conflicting set */
	if (conflict != NULL) {
		if (!ast_addconflict(&conflict, m)) {
			perror("ast_addconflict");
			goto error;
		}
	}

	/*
	 * An ast_mapping is allocated in order to potentially hold
	 * conflicting mappings, if any are found.
	 *
	 * We can't point to an existing ast_mapping in this case,
	 * because a conflict set may not be the same in all DFA states
	 * where the same .to/.token are used.
	 * This is the case for /aa(aa)+/ -> $x; /aaa(aaa)+/ -> $y;
	 * where $y appears in both a conflicting and non-conflicting DFA state.
	 *
	 * If there isn't a conflict, the DFA state point to an existing
	 * mapping. It doesn't matter which one.
	 */
	if (conflict == NULL) {
		assert(m->conflict == NULL);

		state->opaque = m;
	} else {
		struct ast_mapping *new;

		new = malloc(sizeof *new);
		if (new == NULL) {
			goto error;
		}

		new->token    = m->token;
		new->to       = m->to;

		new->fsm      = NULL;
		new->next     = NULL;
		new->conflict = conflict; /* private to this DFA state */

		state->opaque = new;
	}

	return;

error:

	/* XXX: free conflict set */

	state->opaque = NULL;

	return;
}

int
main(int argc, char *argv[])
{
	struct ast *ast;
	void (*out)(const struct ast *ast, FILE *f) = lx_out_c;
	int keep_nfa;

	keep_nfa = 0;

	{
		int c;

		while (c = getopt(argc, argv, "h" "e:t:" "vb:g:l:nx:"), c != -1) {
			switch (c) {
			case 'e':
				prefix.api = optarg;
				break;

			case 't':
				prefix.tok = optarg;
				break;

			case 'b': api_tokbuf  |= lang_tokbuf(optarg);  break;
			case 'g': api_getc    |= lang_getc(optarg);    break;
			case 'x': api_exclude |= lang_exclude(optarg); break;

			case 'l':
				out = language(optarg);
				break;

			case 'n':
				keep_nfa = 1;
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

	if (keep_nfa && out != lx_out_dot) {
		fprintf(stderr, "-n is for .dot output only\n");
		return EXIT_FAILURE;
	}

	if (api_tokbuf && (out != lx_out_c && out != lx_out_h)) {
		fprintf(stderr, "-b is for .c/.h output only\n");
		return EXIT_FAILURE;
	}

	if (api_getc && (out != lx_out_c && out != lx_out_h)) {
		fprintf(stderr, "-c is for .c/.h output only\n");
		return EXIT_FAILURE;
	}

	if (0 != strcmp(prefix.api, "lx_")) {
		prefix.lx = prefix.api;
	}

	ast = lx_parse(stdin);
	if (ast == NULL) {
		return EXIT_FAILURE;
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
	{
		struct ast_zone    *z;
		struct ast_mapping *m;

		for (z = ast->zl; z != NULL; z = z->next) {
			assert(z->fsm == NULL);

			z->fsm = fsm_new();
			if (z->fsm == NULL) {
				perror("fsm_new");
				return EXIT_FAILURE;
			}

			for (m = z->ml; m != NULL; m = m->next) {
				struct fsm_state *s;

				assert(m->fsm != NULL);

				if (!keep_nfa) {
					if (!fsm_minimise(m->fsm)) {
						perror("fsm_minimise");
						return EXIT_FAILURE;
					}
				}

				/* Attach this mapping to each end state for this FSM */
				for (s = m->fsm->sl; s != NULL; s = s->next) {
					if (fsm_isend(m->fsm, s)) {
						assert(s->opaque == NULL);
						s->opaque = m;
					}
				}

				z->fsm = fsm_union(z->fsm, m->fsm);
				if (z->fsm == NULL) {
					perror("fsm_union");
					return EXIT_FAILURE;
				}

#ifndef NDEBUG
				m->fsm = NULL;
#endif
			}

			if (!keep_nfa) {
				if (!fsm_determinise_opaque(z->fsm, carryopaque)) {
					perror("fsm_determinise_opaque");
					return EXIT_FAILURE;
				}
			}
		}
	}

	/*
	 * Semantic checks.
	 */
	{
		struct ast_zone  *z;
		struct fsm_state *s;
		int e;

		assert(ast->zl != NULL);

		/* TODO: check for: no end states (same as no tokens?) */
		/* TODO: check for reserved token names (ERROR, EOF etc) */
		/* TODO: don't forget to indicate zone in error messages */
		/* TODO: check for /abc/ -> $a; /abc/ -> $a; */
		/* TODO: check for /abc+/ -> $a; /abc/ -> $a; */

		e = 0;

		for (z = ast->zl; z != NULL; z = z->next) {
			assert(z->fsm != NULL);
			assert(z->ml  != NULL);

			if (fsm_isend(z->fsm, z->fsm->start)) {
				fprintf(stderr, "start state accepts\n"); /* TODO */
				e = 1;
			}

			/* pick up conflicts flagged by carryopaque() */
			for (s = z->fsm->sl; s != NULL; s = s->next) {
				struct ast_mapping *m;

				if (!fsm_isend(z->fsm, s)) {
					continue;
				}

				assert(s->opaque != NULL);
				m = s->opaque;

				if (m->conflict != NULL) {
					struct mapping_set *p;
					char buf[50]; /* 50 looks reasonable for an on-screen limit */
					int n;

					n = fsm_example(z->fsm, s, buf, sizeof buf);
					if (-1 == n) {
						perror("fsm_example");
						return EXIT_FAILURE;
					}

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

		if (e && out != lx_out_dot) {
			return EXIT_FAILURE;
		}
	}

	/* TODO: free ast */
	/* TODO: free DFA ast_mappings, created in carryopaque, iff making a DFA. i.e. those which have non-NULL conflict sets */
	if (!keep_nfa) {
		struct ast_zone *z;
		struct ast_mapping *m;
		const struct fsm_state *s;

		for (z = ast->zl; z != NULL; z = z->next) {
			for (s = z->fsm->sl; s != NULL; s = s->next) {
				if (!fsm_isend(z->fsm, s)) {
					continue;
				}

				if (s->opaque != NULL) {
					m = s->opaque;

					assert(m->fsm == NULL);

					/* TODO: free m->conflict, allocated in carryopaque */
					(void) m->conflict;
				}
			}
		}
	}

	if (out != NULL) {
		out(ast, stdout);
	}

	return EXIT_SUCCESS;
}

