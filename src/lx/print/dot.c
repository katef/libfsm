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
#include <fsm/options.h>

#include "libfsm/internal.h" /* XXX */

#include "lx/ast.h"
#include "lx/print.h"

static unsigned int anonymous_states = 1;

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
mapping(FILE *f, const struct ast_mapping *m, const struct ast *ast)
{
	assert(m != NULL);
	assert(f != NULL);
	assert(ast != NULL);

	if (m->token != NULL) {
		assert(m->token->s != NULL);

		fprintf(f, "$%s", m->token->s);
	}

	if (m->token != NULL && m->to != NULL) {
		fprintf(f, "<br/>");
	}

	if (m->to != NULL) {
		fprintf(f, "z%u", zindexof(ast, m->to));
	}
}

static int
endleaf_dot(FILE *f, const void *state_opaque, const void *endleaf_opaque)
{
	const struct ast_mapping *m;
	const struct ast *ast;

	assert(f != NULL);
	assert(state_opaque != NULL);
	assert(endleaf_opaque != NULL);

	m = state_opaque;
	ast = endleaf_opaque;

	fprintf(f, "label = <");

/* TODO: would need the fsm for indexing here */
#if 0
	if (!anonymous_states) {
		fprintf(f, "%u<br/>", indexof(fsm, s));
	}
#endif

	if (m->conflict != NULL) {
		const struct mapping_set *p;

		fprintf(f, "<font color=\"red\">");

		for (p = m->conflict; p != NULL; p = p->next) {
			mapping(f, p->m, ast);

			if (p->next != NULL) {
				fprintf(f, "<br/>");
			}
		}

		fprintf(f, "</font>");
	} else {
		mapping(f, m, ast);
	}

	fprintf(f, ">");

	return 0;
}

static void
singlestate(FILE *f, const struct fsm *fsm, const struct ast *ast,
	const struct ast_zone *z, const struct fsm_state *s)
{
	struct ast_mapping *m;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);
	assert(s != NULL);

	if (!fsm_isend(fsm, s)) {
		return;
	}

	m = fsm_getopaque(fsm, s);
	assert(m != NULL);

	if (m->conflict != NULL) {
		const struct mapping_set *p;

		for (p = m->conflict; p != NULL; p = p->next) {
			if (p->m->to != NULL) {
				fprintf(f, "\tz%uS%u -> z%uS%u [ color = red, style = dashed ];\n",
					zindexof(ast, z), indexof(fsm, s),
					zindexof(ast, p->m->to), indexof(p->m->to->fsm, fsm_getstart(p->m->to->fsm)));
			}
		}
	} else {
		if (m->to != NULL) {
			fprintf(f, "\tz%uS%u -> z%uS%u [ color = cornflowerblue, style = dashed ];\n",
				zindexof(ast, z), indexof(fsm, s),
				zindexof(ast, m->to), indexof(m->to->fsm, fsm_getstart(m->to->fsm)));
		}
	}
}

static void
print_zone(FILE *f, const struct ast *ast, const struct ast_zone *z)
{
	size_t i;

	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);
	assert(z->fsm != NULL);
	assert(fsm_getstart(z->fsm) != NULL);

/* XXX: only if not showing transitions between zones:
	fprintf(f, "\tz%u_start [ shape = plaintext ];\n",
		zindexof(ast, z));
	fprintf(f, "\tz%u_start -> z%uS%u;\n",
		zindexof(ast, z),
		zindexof(ast, z),
		indexof(z->fsm, fsm_getstart(z->fsm)));
	fprintf(f, "\n");
*/

	fprintf(f, "\t\n");

	{
		const struct fsm_options *tmp;
		static const struct fsm_options defaults;
		struct fsm_options opt = defaults;
		char p[128];

		tmp = z->fsm->opt;

		(void) sprintf(p, "z%u", zindexof(ast, z));

		opt.anonymous_states  = anonymous_states;
		opt.fragment          = 1; /* XXX */
		opt.consolidate_edges = z->fsm->opt->consolidate_edges;
		opt.comments          = z->fsm->opt->comments;
		opt.prefix            = p;
		opt.endleaf           = endleaf_dot;
		opt.endleaf_opaque    = (void *) ast;

		z->fsm->opt = &opt;

		fsm_print_dot(f, z->fsm);

		z->fsm->opt = tmp;
	}

	for (i = 0; i < z->fsm->statecount; i++) {
		singlestate(f, z->fsm, ast, z, z->fsm->states[i]);
	}

	/* a start state should not accept */
	if (fsm_isend(z->fsm, fsm_getstart(z->fsm))) {
		fprintf(f, "\t\tz%uS%u [ color = red ];\n",
			zindexof(ast, z), indexof(z->fsm, fsm_getstart(z->fsm)));
	}
}

void
lx_print_dot(FILE *f, const struct ast *ast)
{
	const struct ast_zone *z;
	unsigned int zn;

	assert(f != NULL);

	fprintf(f, "digraph %sG {\n", prefix.api);
	fprintf(f, "\trankdir = LR;\n");

	fprintf(f, "\tnode [ shape = circle ];\n");

	if (anonymous_states) {
		fprintf(f, "\tnode [ label = \"\", width = 0.3 ];\n");
	}

	if (ast->zl->next != NULL) {
		size_t i;

		for (z = ast->zl; z != NULL; z = z->next) {
			fprintf(f, "\t\n");
			fprintf(f, "\tsubgraph cluster_z%u {\n", zindexof(ast, z));
			fprintf(f, "\t\tstyle = rounded;\n");
			fprintf(f, "\t\tcolor = gray;\n");

			if (z == ast->global) {
				fprintf(f, "\t\tlabel = <z%u<br/>(global)>;\n", zindexof(ast, z));
			} else {
				fprintf(f, "\t\tlabel = <z%u>;\n", zindexof(ast, z));
			}

			for (i = 0; i < z->fsm->statecount; i++) {
				fprintf(f, "\t\tz%uS%u;\n",
					zindexof(ast, z), (unsigned) i);
			}

			if (z == ast->global) {
				fprintf(f, "\n");
				fprintf(f, "\t\tstart [ shape = plaintext ];\n");
				fprintf(f, "\t\tstart -> z%uS%u;\n",
					zindexof(ast, ast->global),
					indexof(ast->global->fsm, fsm_getstart(ast->global->fsm)));
				fprintf(f, "\t\t{ rank = min; start; }\n");
			}

			fprintf(f, "\t}\n");
		}
	} else {
		fprintf(f, "\n");
		fprintf(f, "\tstart [ shape = plaintext ];\n");
		fprintf(f, "\tstart -> z%uS%u;\n",
			zindexof(ast, ast->global),
			indexof(ast->global->fsm, fsm_getstart(ast->global->fsm)));
		fprintf(f, "\t{ rank = min; start; }\n");
	}

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

		print_zone(f, ast, z);
	}

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

