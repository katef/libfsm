/* $Id$ */

#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <adt/set.h>

#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/cost.h>
#include <fsm/graph.h>
#include <fsm/search.h>

#include <re/re.h>

#include "parser.h"
#include "internal.h"

#include "libfsm/internal.h" /* XXX */

#include "out/out.h"

#include "ast.h"

static
void usage(void)
{
	printf("usage: lx [-h] [-g] [-l <language>] [-p <prefix>]\n");
}

static enum lx_out
language(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum lx_out format;
	} a[] = {
		{ "test", LX_OUT_TEST },
		{ "dot",  LX_OUT_DOT  },
		{ "c",    LX_OUT_C    },
		{ "h",    LX_OUT_H    }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].format;
		}
	}

	fprintf(stderr, "unrecognised output language; valid languages are: "
		"test, dot, c, h\n");
	exit(EXIT_FAILURE);
}

/* TODO: centralise */
static int
set_hasdistinctend(const struct fsm *fsm, const struct state_set *set,
	const struct fsm_state *state)
{
	const struct state_set *s;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = set; s != NULL; s = s->next) {
		if (!fsm_isend(fsm, s->state)) {
			continue;
		}

		if (s->state->opaque != state->opaque) {
			return 1;
		}
	}

	return 0;
}

static void
carryopaque(struct state_set *set, struct fsm *fsm, struct fsm_state *state)
{
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
	 * Because all the accepting states belong to the same regexp, they
	 * should all have the same mapping. So we nominate one to use for the
	 * opaque value and check all other accepting states are the same.
	 */

	for (s = set; s != NULL; s = s->next) {
		if (fsm_isend(fsm, s->state)) {
			state->opaque = s->state->opaque;
			break;
		}
	}

	assert(state->opaque != NULL);

	if (set_hasdistinctend(fsm, set, state)) {
		struct ast_mapping *m;

		m = state->opaque;
		assert(m != NULL);

		for (s = set; s != NULL; s = s->next) {
			if (!fsm_isend(fsm, s->state)) {
				continue;
			}

			if (s->state->opaque == state->opaque) {
				continue;
			}

			assert(s->state->opaque != NULL);

			if (!ast_addconflict(&m->conflict, s->state->opaque)) {
				perror("ast_addconflict");
				goto error;
			}
		}

		if (!ast_addconflict(&m->conflict, state->opaque)) {
			perror("ast_addconflict");
			goto error;
		}
	}

	return;

error:

	state->opaque = NULL;

	return;
}

int
main(int argc, char *argv[])
{
	struct ast *ast;
	enum lx_out format = LX_OUT_C;
	int g;

	g = 0;

	{
		int c;

		while (c = getopt(argc, argv, "hvl:p:g"), c != -1) {
			switch (c) {
			case 'h':
				usage();
				exit(EXIT_SUCCESS);

			case 'l':
				format = language(optarg);
				break;

			case 'p':
				prefix = optarg;
				break;

			case 'g':
				g = 1;
				break;

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

	if (g && format != LX_OUT_DOT) {
		fprintf(stderr, "-g is for dot output only\n");
		return EXIT_FAILURE;
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

			z->fsm = re_new_empty();
			if (z->fsm == NULL) {
				perror("re_new_empty");
				return EXIT_FAILURE;
			}

			for (m = z->ml; m != NULL; m = m->next) {
				struct fsm_state *s;

				assert(m->fsm != NULL);

				if (!g) {
					if (!fsm_minimise(m->fsm)) {
						perror("fsm_minimise");
						return EXIT_FAILURE;
					}
				}

				/* Attach this mapping to each end state for this regexp */
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
			}

			if (!g) {
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
						assert(p->m->token != NULL || p->m->to != NULL);

						if (p->m->token != NULL) {
							fprintf(stderr, "$%s", p->m->token->s);
						}
						if (p->m->token != NULL && p->m->to != NULL) {
							fprintf(stderr, "/");
						}
						if (p->m->to != NULL) {
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

		if (e) {
			return EXIT_FAILURE;
		}
	}

	/* TODO: free ast */

	lx_print(ast, stdout, format);

	return EXIT_SUCCESS;
}

