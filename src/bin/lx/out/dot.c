/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <ctype.h>

#include <fsm/out.h>

#include "libfsm/internal.h" /* XXX */
#include "libfsm/set.h"      /* XXX */
#include "libre/internal.h"	 /* XXX */

#include "../ast.h"
#include "../internal.h"


/* TODO: centralise with libfsm */
static unsigned int
indexof(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_state *s;
	unsigned int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl, i = 1; s != NULL; s = s->next, i++) {
		if (s == state) {
			return i;
		}
	}

	assert(!"unreached");
	return 0;
}

/* TODO: centralise */
static unsigned int
zindexof(const struct ast *ast, const struct ast_zone *zone)
{
	struct ast_zone *z;
	unsigned int i;

	assert(ast != NULL);
	assert(zone != NULL);

	for (z = ast->zl, i = 1; z != NULL; z = z->next, i++) {
		if (z == zone) {
			return i;
		}
	}

	assert(!"unreached");
	return 0;
}

static void singlestate(const struct fsm *fsm, FILE *f, const struct ast *ast,
	const struct ast_zone *z, const struct fsm_state *s) {
	struct ast_mapping *m;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);
	assert(s != NULL);

	if (!fsm_isend(fsm, s)) {
		return;
	}

	assert(s->cl != NULL);
	assert(s->cl->next == NULL);
	assert(s->cl->colour != NULL);

	m = s->cl->colour;

	if (m->token != NULL) {
		assert(m->token->s != NULL);

		fprintf(f, "\t\tz%uS%u -> z%uS%u_tok [ color = gray ];\n",
			zindexof(ast, z), indexof(fsm, s),
			zindexof(ast, z), indexof(fsm, s));

		fprintf(f, "\t\tz%uS%u_tok [ shape = plaintext, label = \"$%s\" ];\n",
			zindexof(ast, z), indexof(fsm, s),
			m->token->s);
	}

	if (m->to == NULL) {
		fprintf(f, "\t\tz%uS%u -> z%uS%u [ color = cornflowerblue, style = dashed ];\n",
			zindexof(ast, z), indexof(fsm, s),
			zindexof(ast, ast->global), indexof(ast->global->re->fsm, ast->global->re->fsm->start));
	} else if (m->to == z) {
		assert(m->to->re != NULL);
		assert(m->to->re->fsm != NULL);
		assert(m->to->re->fsm->start != NULL);

		fprintf(f, "\t\tz%uS%u -> z%uS%u [ color = cornflowerblue, style = dashed ];\n",
			zindexof(ast, z), indexof(fsm, s),
			zindexof(ast, z), indexof(z->re->fsm, z->re->fsm->start));
	} else {
		assert(m->to->re != NULL);
		assert(m->to->re->fsm != NULL);
		assert(m->to->re->fsm->start != NULL);

		fprintf(f, "\t\tz%uS%u -> z%uS%u [ color = cornflowerblue, style = dashed ];\n",
			zindexof(ast, z), indexof(fsm, s),
			zindexof(ast, m->to), indexof(m->to->re->fsm, m->to->re->fsm->start));
	}
}

static void
out_zone(FILE *f, const struct ast *ast, const struct ast_zone *z)
{
	struct fsm_state *s;

	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);
	assert(z->re != NULL);
	assert(z->re->fsm != NULL);
	assert(z->re->fsm->start != NULL);

	fprintf(f, "\tsubgraph cluster_z%u {\n", zindexof(ast, z));
	fprintf(f, "\t\tcolor = gray;\n");

	if (z == ast->global) {
		fprintf(f, "\t\tlabel = <z%u<br/>(global)>;\n", zindexof(ast, z));
	} else {
		fprintf(f, "\t\tlabel = <z%u>;\n", zindexof(ast, z));
	}

	{
		struct fsm_outoptions o = { 0 };
		char prefix[128];

		snprintf(prefix, sizeof prefix, "z%u", zindexof(ast, z));

		o.anonymous_states  = 1;
		o.consolidate_edges = 1;
		o.fragment          = 1;
		o.prefix            = prefix;

		fsm_print(z->re->fsm, f, FSM_OUT_DOT, &o);
	}

	for (s = z->re->fsm->sl; s != NULL; s = s->next) {
		singlestate(z->re->fsm, f, ast, z, s);
	}

	fprintf(f, "\t}\n");
	fprintf(f, "\t\n");
}

void
lx_out_dot(const struct ast *ast, FILE *f)
{
	const struct ast_zone *z;

	assert(f != NULL);

	fprintf(f, "digraph G {\n");

	fprintf(f, "\tnode [ shape = circle, label = \"\" ];\n");
	fprintf(f, "\n");

	fprintf(f, "\tstart [ shape = plaintext ];\n");
	fprintf(f, "\tstart -> z%uS%u [ color = gray ];\n",
		zindexof(ast, ast->global),
		indexof(ast->global->re->fsm, ast->global->re->fsm->start));
	fprintf(f, "\n");

	for (z = ast->zl; z != NULL; z = z->next) {
		out_zone(f, ast, z);
	}

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

