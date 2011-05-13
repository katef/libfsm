/* $Id$ */

#include <assert.h>
#include <stdio.h>

#include <fsm/out.h>

#include "libre/internal.h"	/* XXX */

#include "../ast.h"
#include "../internal.h"


static void
out_zone(FILE *f, const struct ast_zone *z)
{
	const struct ast_mapping *m;
	const struct fsm_outoptions options = { 0, 0, 1, NULL };
	/* TODO: prefix z0, z1 etc */

	assert(f != NULL);
	assert(z != NULL);

	for (m = z->ml; m != NULL; m = m->next) {
		fsm_print(m->re->fsm, f, FSM_OUT_DOT, &options);
	}
}

void
out_dot(const struct ast *ast, FILE *f)
{
	const struct ast_zone *z;

	assert(f != NULL);

	for (z = ast->zl; z != NULL; z = z->next) {
		out_zone(f, z);
	}
}

