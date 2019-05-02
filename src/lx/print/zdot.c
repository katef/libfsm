/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>

#include <adt/set.h>

#include <fsm/pred.h>
#include <fsm/print.h>

#include <print/esc.h>

#include "libfsm/internal.h" /* XXX */

#include "lx/ast.h"
#include "lx/print.h"

/* TODO: centralise */
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

static void
print_zone(FILE *f, const struct ast *ast, const struct ast_zone *z)
{
	size_t i;

	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);
	assert(z->fsm != NULL);
	assert(z->fsm->start != NULL);

	for (i = 0; i < z->fsm->statecount; i++) {
		struct ast_mapping *m;

		if (fsm_isend(z->fsm, z->fsm->states[i])) {
			m = z->fsm->states[i]->opaque;

			if (m->to == NULL) {
				continue;
			}

			fprintf(f, "\tz%u -> z%u;\n", zindexof(ast, z), zindexof(ast, m->to));
		}
	}

	fprintf(f, "\t\n");
}

void
lx_print_zdot(FILE *f, const struct ast *ast)
{
	const struct ast_zone *z;
	unsigned int zn;

	assert(f != NULL);
	assert(ast != NULL);

	fprintf(f, "digraph %sG {\n", prefix.api);
	fprintf(f, "\trankdir = TB;\n");

	fprintf(f, "\tnode [ shape = plaintext];\n");
	fprintf(f, "\n");

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

		fprintf(f, "\tz%u;\n", zindexof(ast, z));

		print_zone(f, ast, z);
	}

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

