/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <ctype.h>

#include <fsm/out.h>
#include <fsm/graph.h>

#include "libre/internal.h"	 /* XXX */
#include "libfsm/internal.h" /* XXX */
#include "libfsm/set.h"      /* XXX */

#include "../ast.h"
#include "../internal.h"


/* TODO: centralise */
static void
out_esctok(FILE *f, const char *s)
{
	const char *p;

	assert(f != NULL);
	assert(s != NULL);

	for (p = s; *p != '\0'; p++) {
		fputc(isalnum(*p) ? toupper(*p) : '_', f);
	}
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

static void escputc(char c, FILE *f) {
	assert(f != NULL);

	if (!isprint(c)) {
		fprintf(f, "\\x%x", (unsigned char) c);
		return;
	}

	switch (c) {
	case '\\': fprintf(f, "\\\\"); return;
	case '\"': fprintf(f, "\\\""); return;
	case '\'': fprintf(f, "\\\'"); return;
	case '\t': fprintf(f, "\\t");  return;
	case '\f': fprintf(f, "\\f");  return;
	case '\r': fprintf(f, "\\r");  return;
	case '\n': fprintf(f, "\\n");  return;

		/* TODO: others */

	default:
		putc(c, f);
	}
}

/* TODO: refactor for when FSM_EDGE_ANY goes; it is an "any" transition if all
 * labels transition to the same state. centralise that, perhaps */
static const struct fsm_state *findany(const struct fsm_state *state) {
	struct state_set *e;
	int i;

	assert(state != NULL);

	for (i = 0; i <= UCHAR_MAX; i++) {
		if (state->edges[i].sl == NULL) {
			return NULL;
		}

		for (e = state->edges[i].sl; e != NULL; e = e->next) {
			if (e->state != state->edges[0].sl->state) {
				return NULL;
			}
		}
	}

	assert(state->edges[0].sl != NULL);

	return state->edges[0].sl->state;
}

/* XXX: centralise */
static int
xset_contains(const struct fsm_state *state, const struct state_set *set)
{
	const struct state_set *s;

	for (s = set; s != NULL; s = s->next) {
		if (s->state == state) {
			return 1;
		}
	}

	return 0;
}

/* Return true if the edges after o contains state */
/* TODO: centralise */
static int contains(struct fsm_edge edges[], int o, struct fsm_state *state) {
	int i;

	assert(edges != NULL);
	assert(state != NULL);

	for (i = o; i <= UCHAR_MAX; i++) {
		if (xset_contains(state, edges[i].sl)) {
			return 1;
		}
	}

	return 0;
}

static void singlecase(FILE *f, const struct ast *ast, const struct ast_zone *z, const struct fsm *fsm, struct fsm_state *state) {
	int i;

	assert(fsm != NULL);
	assert(ast != NULL);
	assert(z != NULL);
	assert(f != NULL);
	assert(state != NULL);

	/* TODO: if greedy and state is an end state, skip this state */

	/* no edges */
	/* TODO: could centralise this with libfsm with internal options passed, perhaps */

	fprintf(f, "\t\t\tswitch (c) {\n");

	for (i = 0; i <= UCHAR_MAX; i++) {
		if (state->edges[i].sl == NULL) {
			continue;
		}

		assert(state->edges[i].sl->state != NULL);
		assert(state->edges[i].sl->next  == NULL);

		fprintf(f, "\t\t\tcase '");
		escputc(i, f);
		fprintf(f, "':");

		/* non-unique states fall through */
		if (contains(state->edges, i + 1, state->edges[i].sl->state)) {
			fprintf(f, "\n");
			continue;
		}

		/* TODO: pass S%u out to maximum state width */
		if (state->edges[i].sl->state != state) {
			fprintf(f, " state = S%u;      continue;\n", indexof(fsm, state->edges[i].sl->state));
		} else {
			fprintf(f, "	          continue;\n");
		}

		/* TODO: if greedy, and fsm_isend(fsm, state->edges[i].sl->state) then:
			fprintf(f, "	     return TOK_%s;\n", state->edges[i].sl->state's token);
		 */
	}

	if (fsm_isend(fsm, state)) {
		struct ast_mapping *m;

		assert(state->cl != NULL);
		assert(state->cl->next == NULL);
		assert(state->cl->colour != NULL);

		m = state->cl->colour;

		fprintf(f, "\t\t\tcase EOF: ");
		if (m->to == NULL) {
			fprintf(f, "lx->getc = NULL; ");
		} else {
			fprintf(f, "                 ");
		}
		fprintf(f, "return ");
		if (m->to != NULL) {
			fprintf(f, "lx->z = z%u, ", zindexof(ast, m->to));
		}
		if (m->token != NULL) {
			fprintf(f, "TOK_");
			out_esctok(f, m->token->s);
		} else {
			fprintf(f, "lx->z(lx)");
		}
		fprintf(f, ";\n");

		fprintf(f, "\t\t\tdefault:  lx->ungetc(c);   return ");
		if (m->to != NULL) {
			fprintf(f, "lx->z = z%u, ", zindexof(ast, m->to));
		}
		if (m->token != NULL) {
			fprintf(f, "TOK_");
			out_esctok(f, m->token->s);
		} else {
			fprintf(f, "lx->z(lx)");
		}
		fprintf(f, ";\n"); /* TODO: colour for token */
	} else {
		fprintf(f, "\t\t\tcase EOF: lx->getc = NULL; return %s;\n",
			z == ast->global ? "TOK_EOF" : "TOK_ERROR");
		fprintf(f, "\t\t\tdefault:  lx->getc = NULL; return TOK_ERROR;\n");
	}

	fprintf(f, "\t\t\t}\n");
}

/* TODO: centralise with libfsm */
static void stateenum(FILE *f, const struct fsm *fsm, struct fsm_state *sl) {
	struct fsm_state *s;
	int i;

	fprintf(f, "\tenum {\n");
	fprintf(f, "\t\t");

	for (s = sl, i = 1; s != NULL; s = s->next, i++) {
		fprintf(f, "S%u", indexof(fsm, s));
		if (s->next != NULL) {
			fprintf(f, ", ");
		}

		if (i % 10 == 0) {
			fprintf(f, "\n");
			fprintf(f, "\t\t");
		}
	}

	fprintf(f, "\n");
	fprintf(f, "\t} state;\n");
}

static void out_cfrag(const struct fsm *fsm, const struct ast_zone *z, FILE *f,
	const struct ast *ast) {
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(fsm_isdfa(fsm));
	assert(z != NULL);
	assert(ast != NULL);
	assert(f != NULL);

	/* TODO: prerequisite that the FSM is a DFA */

	fprintf(f, "\t\tswitch (state) {\n");

	for (s = fsm->sl; s != NULL; s = s->next) {
		fprintf(f, "\t\tcase S%u:\n", indexof(fsm, s));
		singlecase(f, ast, z, fsm, s);

		if (s->next != NULL) {
			fprintf(f, "\n");
		}
	}

	fprintf(f, "\t\t}\n");
}

static void
out_zone(FILE *f, const struct ast *ast, const struct ast_zone *z)
{
	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);

	fprintf(f, "static enum lx_token z%u(struct lx *lx) {\n", zindexof(ast, z));
	fprintf(f, "\tint c;\n");
	fprintf(f, "\n");

	stateenum(f, z->re->fsm, z->re->fsm->sl);
	fprintf(f, "\n");

	fprintf(f, "\tassert(lx != NULL);\n");
	fprintf(f, "\n");

	assert(z->re->fsm->start != NULL);
	fprintf(f, "\tstate = S%u;\n", indexof(z->re->fsm, z->re->fsm->start));
	fprintf(f, "\n");

	fprintf(f, "\twhile ((c = lx->getc(lx->opaque)) != EOF) {\n");

	out_cfrag(z->re->fsm, z, f, ast);

	fprintf(f, "\t}\n");
	fprintf(f, "}\n\n");
}

void
out_c(const struct ast *ast, FILE *f)
{
	const struct ast_zone *z;

	assert(f != NULL);

	fprintf(f, "/* Generated by lx */\n");	/* TODO: date, input etc */
	fprintf(f, "\n");

	fprintf(f, "#include <assert.h>\n");
	fprintf(f, "#include <stdio.h>\n");
	fprintf(f, "\n");

	fprintf(f, "#include LX_HEADER\n");
	fprintf(f, "\n");

	for (z = ast->zl; z != NULL; z = z->next) {
		out_zone(f, ast, z);
	}

	fprintf(f, "enum lx_token lx_nexttoken(struct lx *lx) {\n");

	fprintf(f, "\tenum lx_token t;");
	fprintf(f, "\n");
	fprintf(f, "\tassert(lx != NULL);\n");
	fprintf(f, "\n");

	fprintf(f, "\tif (lx->getc == NULL) {\n");
	fprintf(f, "\t\treturn TOK_EOF;\n");
	fprintf(f, "\t}\n");
	fprintf(f, "\n");

	fprintf(f, "\tif (lx->z == NULL) {\n");
	fprintf(f, "\t\tlx->z = z%u;\n", zindexof(ast, ast->global));
	fprintf(f, "\t}\n");
	fprintf(f, "\n");

	fprintf(f, "\tt = lx->z(lx);\n");
	fprintf(f, "\n");
	fprintf(f, "\tlx->z = z%u;\n", zindexof(ast, ast->global));
	fprintf(f, "\n");
	fprintf(f, "\treturn t;\n");

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

