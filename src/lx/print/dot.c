/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include "libfsm/internal.h" /* XXX */

#include "lx/lx.h"
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
accept_dot(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	const struct ast_mapping *m;
	const struct ast *ast;
	fsm_state_t s;

	assert(f != NULL);
	assert(opt != NULL);
	assert(state_metadata->end_ids != NULL);
	assert(state_metadata->end_id_count > 0);
	assert(lang_opaque != NULL);
	assert(hook_opaque != NULL);

	ast = hook_opaque;
	s = * (fsm_state_t *) lang_opaque;

	fprintf(f, "label = <");

	if (!anonymous_states) {
		fprintf(f, "%u<br/>", s);
	}

	if (state_metadata->end_id_count == 1) {
		m = ast_getendmappingbyendid(state_metadata->end_ids[0]);
		mapping(f, m, ast);
	} else {
		size_t i;

		fprintf(f, "<font color=\"red\">");

		for (i = 0; i < state_metadata->end_id_count; i++) {
			m = ast_getendmappingbyendid(state_metadata->end_ids[i]);
			mapping(f, m, ast);

			if (i + 1 < state_metadata->end_id_count) {
				fprintf(f, "<br/>");
			}
		}

		fprintf(f, "</font>");
	}

	fprintf(f, ">");

	return 0;
}

static void
singlestate(FILE *f, const struct fsm *fsm, const struct ast *ast,
	const struct ast_zone *z, fsm_state_t s)
{
	struct ast_mapping *m;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);

	if (!fsm_isend(fsm, s)) {
		return;
	}

	m = ast_getendmapping(fsm, s);
	if (LOG()) {
		fprintf(stderr, "singlestate: ast_getendmapping for state %d: %p (dot)\n",
		    s, (void *)m);
	}
	assert(m != NULL);

	if (m->to != NULL) {
		fsm_state_t start;

		(void) fsm_getstart(m->to->fsm, &start);

		fprintf(f, "\tz%uS%u -> z%uS%u [ color = %s, style = dashed ];\n",
			zindexof(ast, z), s,
			zindexof(ast, m->to), start,
			fsm_endid_count(fsm, s) > 1 ? "red" : "cornflowerblue");
	}
}

static void
print_zone(FILE *f, const struct ast *ast, const struct ast_zone *z,
	const struct fsm_options *opt)
{
	fsm_state_t start, i;

	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);
	assert(z->fsm != NULL);
	assert(opt != NULL);

	(void) fsm_getstart(z->fsm, &start);

/* XXX: only if not showing transitions between zones:
	fprintf(f, "\tz%u_start [ shape = plaintext ];\n",
		zindexof(ast, z));
	fprintf(f, "\tz%u_start -> z%uS%u;\n",
		zindexof(ast, z),
		zindexof(ast, z),
		start);
	fprintf(f, "\n");
*/

	fprintf(f, "\t\n");

	{
		static const struct fsm_hooks defaults;
		struct fsm_hooks hooks = defaults;
		struct fsm_options o = *opt;

		char p[128];

		(void) sprintf(p, "z%u", zindexof(ast, z));

		o.prefix = p;
		o.fragment = 1;

 		hooks.accept      = accept_dot;
		hooks.hook_opaque = (void *) ast;

		fsm_print(f, z->fsm, &o, &hooks, FSM_PRINT_DOT);
	}

	for (i = 0; i < z->fsm->statecount; i++) {
		singlestate(f, z->fsm, ast, z, i);
	}

	/* a start state should not accept */
	if (fsm_isend(z->fsm, start)) {
		fprintf(f, "\t\tz%uS%u [ color = red ];\n",
			zindexof(ast, z), start);
	}
}

void
lx_print_dot(FILE *f, const struct ast *ast, const struct fsm_options *opt)
{
	const struct ast_zone *z;
	unsigned int zn;
	fsm_state_t start;

	assert(f != NULL);
	assert(opt != NULL);

	(void) fsm_getstart(ast->global->fsm, &start);

	fprintf(f, "digraph %sG {\n", prefix.api);
	fprintf(f, "\trankdir = LR;\n");

	fprintf(f, "\tnode [ shape = circle ];\n");

	if (anonymous_states) {
		fprintf(f, "\tnode [ label = \"\", width = 0.3 ];\n");
	}

	if (ast->zl->next != NULL) {
		fsm_state_t i;

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
					start);
				fprintf(f, "\t\t{ rank = min; start; }\n");
			}

			fprintf(f, "\t}\n");
		}
	} else {
		fprintf(f, "\n");
		fprintf(f, "\tstart [ shape = plaintext ];\n");
		fprintf(f, "\tstart -> z%uS%u;\n",
			zindexof(ast, ast->global),
			start);
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

		print_zone(f, ast, z, opt);
	}

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

