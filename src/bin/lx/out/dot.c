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


/* TODO: centralise */
static void escputc(int c, FILE *f) {
	assert(f != NULL);

	switch (c) {
	case FSM_EDGE_EPSILON:
		fprintf(f, "E");	/* TODO */
		return;

#if 0
	case '\\':
		fprintf(f, "\\\\");
		return;

	case '\n':
		fprintf(f, "\\n");
		return;

	case '\r':
		fprintf(f, "\\r");
		return;

	case '\f':
		fprintf(f, "\\f");
		return;

	case '\v':
		fprintf(f, "\\v");
		return;

	case '\t':
		fprintf(f, "\\t");
		return;

	case ' ':
		/* fprintf(f, "&lsquo; &rsquo;"); */
		fprintf(f, "' '");
		return;

	case '\"':
		/* fprintf(f, "&lsquo;\\\"&rsquo;"); */
		fprintf(f, "'\\\"'");
		return;

	case '\'':
		/* fprintf(f, "&lsquo;\\'&rsquo;"); */
		fprintf(f, "'\\''");
		return;

	case '.':
		/* fprintf(f, "&lsquo;.&rsquo;"); */
		fprintf(f, "'.'");
		return;

	case '|':
		/* fprintf(f, "&lsquo;|&rsquo;"); */
		fprintf(f, "'|'");
		return;
#endif

	case '<':
		/* fprintf(f, "&lsquo;<&rsquo;"); */
		fprintf(f, "'<'");
		return;

	case '>':
		/* fprintf(f, "&lsquo;>&rsquo;"); */
		fprintf(f, "'>'");
		return;

	case '&':
		/* fprintf(f, "&lsquo;&amp;&rsquo;"); */
		fprintf(f, "'amp'");
		return;

	case ']':
		/* fprintf(f, "&lsquo;=&rsquo;"); */
		fprintf(f, "'cs'");
		return;

	case '[':
		/* fprintf(f, "&lsquo;=&rsquo;"); */
		fprintf(f, "'os'");
		return;

	case '=':
		/* fprintf(f, "&lsquo;=&rsquo;"); */
		fprintf(f, "equals");
		return;

	/* TODO: others */
	}

	if (!isprint(c)) {
		fprintf(f, "0x%x", (unsigned char) c);
		return;
	}

	putc(c, f);
}

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

	fprintf(f, "\t\tz%uS%u;\n",
		zindexof(ast, z), indexof(fsm, s));

	if (fsm->start == s) {
		fprintf(f, "\t\t{ rank = min; z%uS%u; }\n",
			zindexof(ast, z), indexof(fsm, s));
	}

	if (!fsm_isend(fsm, s)) {
		return;
	}

	assert(s->cl != NULL);
	assert(s->cl->next == NULL);
	assert(s->cl->colour != NULL);

	m = s->cl->colour;

	if (m->token != NULL) {
		assert(m->token->s != NULL);

		fprintf(f, "\t\tz%uS%u [ shape = doublecircle, label = \"$%s\" ];\n",
			zindexof(ast, z), indexof(fsm, s),
			m->token->s);
	}
}

static void edgesstate(const struct fsm *fsm, FILE *f, const struct ast *ast,
	const struct ast_zone *z, const struct fsm_state *s) {
	struct ast_mapping *m;
	struct state_set *e;
	int i;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);
	assert(s != NULL);

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		for (e = s->edges[i].sl; e != NULL; e = e->next) {
			assert(e->state != NULL);
			fprintf(f, "\t\tz%uS%u -> z%uS%u [ label = <",
				zindexof(ast, z), indexof(fsm, s),
				zindexof(ast, z), indexof(fsm, e->state));

			escputc(i, f);

			fprintf(f, "> ];\n");
		}
	}

	if (!fsm_isend(fsm, s)) {
		return;
	}

	assert(s->cl != NULL);
	assert(s->cl->next == NULL);
	assert(s->cl->colour != NULL);

	m = s->cl->colour;

	if (m->to == NULL) {
		fprintf(f, "\t\tz%uS%u -> z%uS%u [ style = dashed ];\n",
			zindexof(ast, z), indexof(fsm, s),
			zindexof(ast, ast->global), indexof(ast->global->re->fsm, ast->global->re->fsm->start));
	} else if (m->to == z) {
		assert(m->to->re != NULL);
		assert(m->to->re->fsm != NULL);
		assert(m->to->re->fsm->start != NULL);

		fprintf(f, "\t\tz%uS%u -> z%uS%u [ style = dashed ];\n",
			zindexof(ast, z), indexof(fsm, s),
			zindexof(ast, z), indexof(z->re->fsm, z->re->fsm->start));
	} else {
		assert(m->to->re != NULL);
		assert(m->to->re->fsm != NULL);
		assert(m->to->re->fsm->start != NULL);

		fprintf(f, "\t\tz%uS%u -> z%uS%u [ style = dashed ];\n",
			zindexof(ast, z), indexof(fsm, s),
			zindexof(ast, m->to), indexof(m->to->re->fsm, m->to->re->fsm->start));
	}
}

static void
out_zone(FILE *f, const struct ast *ast, const struct ast_zone *z)
{
	const struct fsm_state *s;
	/* TODO: prefix z0, z1 etc */

	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);
	assert(z->re != NULL);
	assert(z->re->fsm != NULL);
	assert(z->re->fsm->start != NULL);

	fprintf(f, "\tsubgraph cluster_z%u {\n", zindexof(ast, z));

	if (z == ast->global) {
		fprintf(f, "\t\tlabel = <z%u<br/>(global)>;\n", zindexof(ast, z));
	} else {
		fprintf(f, "\t\tlabel = <z%u>;\n", zindexof(ast, z));
	}

	for (s = z->re->fsm->sl; s != NULL; s = s->next) {
		singlestate(z->re->fsm, f, ast, z, s);
	}

	fprintf(f, "\t}\n");
	fprintf(f, "\t\n");
}

static void
out_zoneedges(FILE *f, const struct ast *ast, const struct ast_zone *z)
{
	const struct fsm_state *s;
	/* TODO: prefix z0, z1 etc */

	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);
	assert(z->re != NULL);
	assert(z->re->fsm != NULL);
	assert(z->re->fsm->start != NULL);

	for (s = z->re->fsm->sl; s != NULL; s = s->next) {
		edgesstate(z->re->fsm, f, ast, z, s);
	}
}

void
out_dot(const struct ast *ast, FILE *f)
{
	const struct ast_zone *z;

	assert(f != NULL);

	fprintf(f, "digraph G {\n");

	fprintf(f, "\tnode [ shape = circle, label = \"\" ];\n");
	fprintf(f, "\n");

	fprintf(f, "\tstart [ shape = plaintext ];\n");
	fprintf(f, "\tstart -> z%uS%u\n",
		zindexof(ast, ast->global), indexof(ast->global->re->fsm, ast->global->re->fsm->start));
	fprintf(f, "\n");

	for (z = ast->zl; z != NULL; z = z->next) {
		out_zone(f, ast, z);
	}

	fprintf(f, "\n");

	for (z = ast->zl; z != NULL; z = z->next) {
		out_zoneedges(f, ast, z);
	}

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

