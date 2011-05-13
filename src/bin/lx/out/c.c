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

static void singlecase(FILE *f, const struct fsm *fsm, struct fsm_state *state) {
	const struct fsm_state *to;
	int i;

	assert(fsm != NULL);
	assert(f != NULL);
	assert(state != NULL);

	/* TODO: move this out into a count function */
	for (i = 0; i <= UCHAR_MAX; i++) {
		if (state->edges[i].sl != NULL) {
			break;
		}
	}

	/* no edges */
	/* TODO: could centralise this with libfsm with internal options passed, perhaps */
	if (i > UCHAR_MAX) {
		fprintf(f, "\t\t\treturn TOK_ERROR;\n");
		return;
	}

	fprintf(f, "\t\t\tswitch (c) {\n");

	/* "any" edge */
	to = findany(state);

	/* usual case */
	if (to == NULL) {
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
				fprintf(f, " state = S%u; continue;\n", indexof(fsm, state->edges[i].sl->state));
			} else {
				fprintf(f, "	     continue;\n");
			}
		}
	}

	if (to != NULL) {
		fprintf(f, "\t\t\tdefault:  state = S%u; continue;\n", indexof(fsm, to));
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

static void endstates(FILE *f, const struct ast *ast, const struct fsm *fsm, struct fsm_state *sl) {
	struct fsm_state *s;

	assert(f != NULL);
	assert(ast != NULL);
	assert(fsm != NULL);
	assert(fsm_hasend(fsm));

	/* usual case */
	fprintf(f, "\t/* end states */\n");
	fprintf(f, "\tswitch (state) {\n");
	for (s = sl; s != NULL; s = s->next) {
		if (!fsm_isend(fsm, s)) {
			continue;
		}

/* TODO: TOK_xyz, not indexof */
/* TODO: and change zone, if .to is non-NULL */
/* TODO: assert only one zone (if any) */
/* TODO: assert exactly one token type */
		fprintf(f, "\tcase S%u: ", indexof(fsm, s));

/* TODO: work with s->cl from here */
assert(s->cl != NULL);
assert(s->cl->next == NULL);

/* TODO: so... s->cl->colour should point to an ast_mapping struct */
/* TODO: for now it's an ast_mapping struct instead... */

{
struct ast_mapping *m;

m = s->cl->colour;
assert(m != NULL);

if (m->to != NULL) {
	fprintf(f, "lx->z = z%u; ", zindexof(ast, m->to));
} else {
	fprintf(f, "            ");
}

if (m->token != NULL) {
	fprintf(f, "return TOK_");
	out_esctok(f, m->token->s);
	fprintf(f, ";\n");
} else {
	fprintf(f, "return lx->z(lx);\n");
}


}

	}
	fprintf(f, "\tdefault: return TOK_EOF; /* unexpected EOF */\n");
	fprintf(f, "\t}\n");
}

static void out_cfrag(const struct fsm *fsm, FILE *f) {
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(fsm_isdfa(fsm));
	assert(f != NULL);

	/* TODO: prerequisite that the FSM is a DFA */

	fprintf(f, "\t\tswitch (state) {\n");
	for (s = fsm->sl; s != NULL; s = s->next) {
		fprintf(f, "\t\tcase S%u:\n", indexof(fsm, s));
		singlecase(f, fsm, s);

		if (s->next != NULL) {
			fprintf(f, "\n");
		}
	}
	fprintf(f, "\t\t}\n");
}

static void
out_zone(FILE *f, const struct ast *ast, const struct ast_zone *z, unsigned int i)
{
	/* TODO: prefix z0, z1 etc */

	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);

	fprintf(f, "static enum lx_token z%u(struct lx *lx) {\n", i);
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

	out_cfrag(z->re->fsm, f);

	fprintf(f, "\t}\n");
	fprintf(f, "\n");

	endstates(f, ast, z->re->fsm, z->re->fsm->sl);

	fprintf(f, "}\n\n");
}

void
out_c(const struct ast *ast, FILE *f)
{
	const struct ast_zone *z;
	unsigned int i;

	assert(f != NULL);

	fprintf(f, "/* Generated by lx */\n");	/* TODO: date, input etc */
	fprintf(f, "\n");

	fprintf(f, "#include <assert.h>\n");
	fprintf(f, "\n");

	for (z = ast->zl, i = 0; z != NULL; z = z->next, i++) {
		out_zone(f, ast, z, i);
	}

	fprintf(f, "enum lx_token lx_nexttoken(struct lx *lx) {\n");
	fprintf(f, "\tassert(lx != NULL);\n");
	fprintf(f, "\n");
	fprintf(f, "\tif (lx->z == NULL) {\n");
	fprintf(f, "\t\tlx->z = z%u;\n", zindexof(ast, ast->global));
	fprintf(f, "\t}\n");
	fprintf(f, "\n");
	fprintf(f, "\treturn lx->z(lx);\n");
	fprintf(f, "}\n");
}

