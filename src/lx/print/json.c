/*
 * Copyright 2019 Jamey Sharp
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include "libfsm/internal.h" /* XXX */

#include "lx/ast.h"
#include "lx/print.h"

/* TODO: centralise with libfsm */
static unsigned int
indexof(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_state *s;
	unsigned int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl, i = 0; s != NULL; s = s->next, i++) {
		if (s == state) {
			return i;
		}
	}

	assert(!"unreached");
	return 0;
}

static unsigned int
zindexof(const struct ast *ast, const struct ast_zone *zone)
{
	struct ast_zone *z;
	unsigned int i;

	assert(ast != NULL);
	assert(zone != NULL);

	for (z = ast->zl, i = 0; z != NULL; z = z->next, i++) {
		if (z == zone) {
			return i;
		}
	}

	assert(!"unreached");
	return 0;
}

static int
print_zone(FILE *f, const struct ast *ast, const struct ast_zone *z)
{
	struct fsm_state *s, *st;
	int ret;

	assert(f != NULL);
	assert(z != NULL);
	assert(z->fsm != NULL);
	assert(fsm_all(z->fsm, fsm_isdfa));
	assert(ast != NULL);

	fprintf(f, "    {\n");
	fprintf(f, "      \"initial_state\": %u,\n", indexof(z->fsm, z->fsm->start));
	fprintf(f, "      \"states\": [\n");

	for (s = z->fsm->sl; s != NULL; s = s->next) {
		struct fsm_edge *e;
		struct set_iter it;

		fprintf(f, "        {\n");

		e = set_first(s->edges, &it);

		if (fsm_isend(z->fsm, s)) {
			const struct ast_mapping *m = s->opaque;
			assert(m != NULL);

			if (m->token != NULL) {
				fprintf(f, "          \"token\": \"$%s\",\n", m->token->s);
			}
			if (m->to != NULL) {
				fprintf(f, "          \"next_zone\": %u,\n", zindexof(ast, m->to));
			}

			fprintf(f, "          \"accepts\": true%s\n", e != NULL ? "," : "");
		}

		if (e != NULL) {
			int first = 1;
			fprintf(f, "          \"transitions\": [");
			for (; e != NULL; e = set_next(&it)) {
				struct set_iter jt;
				for (st = set_first(e->sl, &jt); st != NULL; st = set_next(&jt)) {
					fprintf(f, "%s\n            { \"symbol\": %u, \"target\": %u }",
						first ? "" : ",",
						e->symbol,
						indexof(z->fsm, st));
					first = 0;
				}
			}
			fprintf(f, "\n          ]\n");
		}

		fprintf(f, "        }%s\n", s->next ? "," : "");
	}

	fprintf(f, "      ]\n");
	fprintf(f, "    }%s\n", z->next ? "," : "");

	return 0;
}

void
lx_print_json(FILE *f, const struct ast *ast)
{
	const struct ast_zone *z;
	unsigned int zn;

	assert(f != NULL);

	for (z = ast->zl; z != NULL; z = z->next) {
		if (!fsm_all(z->fsm, fsm_isdfa)) {
			errno = EINVAL;
			return;
		}
	}

	fprintf(f, "{\n");
	fprintf(f, "  \"prefix\": { \"api\": \"%s\", \"tok\": \"%s\", \"lx\": \"%s\" },\n",
		prefix.api, prefix.tok, prefix.lx);
	fprintf(f, "  \"initial_zone\": %u,\n", zindexof(ast, ast->global));
	fprintf(f, "  \"zones\": [\n");

	if (print_progress) {
		zn = 0;
	}

	for (z = ast->zl; z != NULL; z = z->next) {
		if (print_progress) {
			if (important(zn)) {
				fprintf(stderr, " z%u", zn);
			}
			zn++;
		}

		if (-1 == print_zone(f, ast, z)) {
			return; /* XXX: handle error */
		}
	}

	fprintf(f, "  ]\n");
	fprintf(f, "}\n");
}
