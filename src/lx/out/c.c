/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include <adt/set.h>

#include <fsm/out.h>
#include <fsm/pred.h>
#include <fsm/graph.h>

#include "libfsm/internal.h" /* XXX */

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

static void
escputc(char c, FILE *f)
{
	assert(f != NULL);

	if (!isprint(c) && !isspace(c)) {
		fprintf(f, "\\x%x", (unsigned char) c);
		return;
	}

	switch (c) {
	case '\\': fprintf(f, "\\\\"); return;
	case '\"': fprintf(f, "\\\""); return;
	case '\'': fprintf(f, "\\\'"); return;
	case '\t': fprintf(f, "\\t");  return;
	case '\n': fprintf(f, "\\n");  return;
	case '\v': fprintf(f, "\\v");  return;
	case '\f': fprintf(f, "\\f");  return;
	case '\r': fprintf(f, "\\r");  return;

		/* TODO: others */

	default:
		putc(c, f);
	}
}

/* XXX: centralise */
static int
xset_contains(const struct state_set *set, const struct fsm_state *state)
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
static int
contains(struct fsm_edge edges[], int o, struct fsm_state *state)
{
	int i;

	assert(edges != NULL);
	assert(state != NULL);

	for (i = o; i <= UCHAR_MAX; i++) {
		if (xset_contains(edges[i].sl, state)) {
			return 1;
		}
	}

	return 0;
}

static void
singlecase(FILE *f, const struct ast *ast, const struct ast_zone *z,
	const struct fsm *fsm, struct fsm_state *state)
{
	struct fsm_state *mode;
	int i;

	assert(fsm != NULL);
	assert(ast != NULL);
	assert(z != NULL);
	assert(f != NULL);
	assert(state != NULL);

	/* TODO: assert that there are never no edges? */
	/* TODO: if greedy and state is an end state, skip this state */

	/* TODO: could centralise this with libfsm with internal options passed, perhaps */

	fprintf(f, "\t\t\tswitch (c) {\n");

	mode = fsm_iscompletestate(fsm, state) ? fsm_findmode(state) : NULL;

	for (i = 0; i <= UCHAR_MAX; i++) {
		if (state->edges[i].sl == NULL) {
			continue;
		}

		assert(state->edges[i].sl->state != NULL);
		assert(state->edges[i].sl->next  == NULL);

		if (state->edges[i].sl->state == mode) {
			continue;
		}

		fprintf(f, "\t\t\tcase '");
		escputc(i, f);
		fprintf(f, "':");

		/* non-unique states fall through */
		if (contains(state->edges, i + 1, state->edges[i].sl->state)) {
			fprintf(f, "\n");
			continue;
		}

		/* TODO: pad S%u out to maximum state width */
		if (state->edges[i].sl->state != state) {
			fprintf(f, " state = S%u;      continue;\n", indexof(fsm, state->edges[i].sl->state));
		} else {
			fprintf(f, "	          continue;\n");
		}

		/* TODO: if greedy, and fsm_isend(fsm, state->edges[i].sl->state) then:
			fprintf(f, "	     return TOK_%s;\n", state->edges[i].sl->state's token);
		 */
	}

	if (mode != NULL) {
		/* TODO: state-change as for typical cases */
		/* TODO: i think... */
		fprintf(f, "\t\t\tdefault:  state = S%u;     continue;\n",  indexof(fsm, mode));

		goto done;
	}

	if (!fsm_isend(fsm, state)) {
		/* XXX: don't need this if complete */
		fprintf(f, "\t\t\tdefault:  lx->getc = NULL; return TOK_ERROR;\n");
	} else {
		const struct ast_mapping *m;

		m = state->opaque;

		assert(m != NULL);

		/* XXX: don't need this if complete */
		fprintf(f, "\t\t\tdefault:  lx->ungetc(c, lx->opaque); return ");
		if (m->to != NULL) {
			fprintf(f, "lx->z = z%u, ", zindexof(ast, m->to));
		}
		if (m->token == NULL && m->to == NULL) {
			fprintf(f, "TOK_SKIP");
		} else if (m->token != NULL) {
			fprintf(f, "TOK_");
			out_esctok(f, m->token->s);
		} else {
			fprintf(f, "lx->z(lx)");
		}
		fprintf(f, ";\n");
	}

done:

	fprintf(f, "\t\t\t}\n");
}

/* TODO: centralise with libfsm */
static void
stateenum(FILE *f, const struct fsm *fsm, struct fsm_state *sl)
{
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

static void
out_proto(FILE *f, const struct ast *ast, const struct ast_zone *z)
{
	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);

	fprintf(f, "static enum lx_token z%u(struct lx *lx);\n", zindexof(ast, z));
}

static void
out_zone(FILE *f, const struct ast *ast, const struct ast_zone *z)
{
	assert(f != NULL);
	assert(z != NULL);
	assert(z->fsm != NULL);
	assert(fsm_predicate(z->fsm, fsm_isdfa));
	assert(ast != NULL);

	/* TODO: prerequisite that the FSM is a DFA */

	fprintf(f, "static enum lx_token\n");
	fprintf(f, "z%u(struct lx *lx)\n", zindexof(ast, z));
	fprintf(f, "{\n");
	fprintf(f, "\tint c;\n");
	fprintf(f, "\n");

	stateenum(f, z->fsm, z->fsm->sl);
	fprintf(f, "\n");

	fprintf(f, "\tassert(lx != NULL);\n");
	fprintf(f, "\tassert(lx->getc != NULL);\n");
	fprintf(f, "\n");

	assert(z->fsm->start != NULL);
	fprintf(f, "\tstate = S%u;\n", indexof(z->fsm, z->fsm->start));
	fprintf(f, "\n");

	{
		struct fsm_state *s;

		fprintf(f, "\twhile (c = lx->getc(lx->opaque), c != EOF) {\n");

		fprintf(f, "\t\tswitch (state) {\n");

		for (s = z->fsm->sl; s != NULL; s = s->next) {
			fprintf(f, "\t\tcase S%u:\n", indexof(z->fsm, s));
			singlecase(f, ast, z, z->fsm, s);

			if (s->next != NULL) {
				fprintf(f, "\n");
			}
		}

		fprintf(f, "\t\t}\n");

		fprintf(f, "\t}\n");
	}

	fprintf(f, "\n");

	{
		struct fsm_state *s;

		fprintf(f, "\tlx->getc = NULL;\n");

		fprintf(f, "\n");

		fprintf(f, "\tswitch (state) {\n");

		for (s = z->fsm->sl; s != NULL; s = s->next) {
			const struct ast_mapping *m;

			if (!fsm_isend(z->fsm, s)) {
				continue;
			}

			m = s->opaque;

			assert(m != NULL);

			fprintf(f, "\tcase S%u: return ", indexof(z->fsm, s));

			/* note: no point in changing zone here, because getc is now NULL */

			if (m->token == NULL) {
				fprintf(f, "TOK_EOF;\n");
			} else {
				/* TODO: maybe make a printf-like little language to simplify this */
				fprintf(f, "TOK_");
				out_esctok(f, m->token->s);
				fprintf(f, ";\n");
			}
		}

		fprintf(f, "\tdefault: return TOK_ERROR;\n");

		fprintf(f, "\t}\n");
	}

	fprintf(f, "}\n\n");
}

void
lx_out_c(const struct ast *ast, FILE *f)
{
	const struct ast_zone *z;

	assert(f != NULL);

	for (z = ast->zl; z != NULL; z = z->next) {
		if (!fsm_predicate(z->fsm, fsm_isdfa)) {
			errno = EINVAL;
			return;
		}
	}

	fprintf(f, "/* Generated by lx */\n");	/* TODO: date, input etc */
	fprintf(f, "\n");

	fprintf(f, "#include <assert.h>\n");
	fprintf(f, "#include <stdio.h>\n");
	fprintf(f, "\n");

	fprintf(f, "#include LX_HEADER\n");
	fprintf(f, "\n");

	for (z = ast->zl; z != NULL; z = z->next) {
		out_proto(f, ast, z);
	}

	fprintf(f, "\n");

	for (z = ast->zl; z != NULL; z = z->next) {
		out_zone(f, ast, z);
	}

	{
		struct ast_token *t;

		fprintf(f, "const char *\n");
		fprintf(f, "lx_name(enum lx_token t)\n");
		fprintf(f, "{\n");

		fprintf(f, "\tswitch (t) {\n");

		for (t = ast->tl; t != NULL; t = t->next) {
			fprintf(f, "\tcase TOK_");
			out_esctok(f, t->s);
			fprintf(f, ": return \"");
			out_esctok(f, t->s);
			fprintf(f, "\";\n");
		}

		fprintf(f, "\tcase TOK_EOF:   return \"EOF\";\n");
		fprintf(f, "\tcase TOK_SKIP:  return \"SKIP\";\n");
		fprintf(f, "\tcase TOK_ERROR: return \"ERROR\";\n");

		fprintf(f, "\tdefault: return \"?\";\n");

		fprintf(f, "\t}\n");

		fprintf(f, "}\n");
	}

	fprintf(f, "\n");

	{
		fprintf(f, "enum lx_token\n");
		fprintf(f, "lx_nexttoken(struct lx *lx)\n");
		fprintf(f, "{\n");

		fprintf(f, "\tenum lx_token t;\n");
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
	}

	fprintf(f, "\n");
}

