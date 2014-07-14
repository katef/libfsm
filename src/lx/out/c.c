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

	mode = fsm_iscomplete(fsm, state) ? fsm_findmode(state) : NULL;

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
		fprintf(f, "\t\t\tdefault:  lx->lgetc = NULL; return TOK_ERROR;\n");
	} else {
		const struct ast_mapping *m;

		m = state->opaque;

		assert(m != NULL);

		/* XXX: don't need this if complete */
		fprintf(f, "\t\t\tdefault:  lx_ungetc(lx, c); return ");
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
out_lgetc(FILE *f)
{
	fprintf(f, "int\n");
	fprintf(f, "lx_fgetc(struct lx *lx)\n");
	fprintf(f, "{\n");
	fprintf(f, "\tassert(lx != NULL);\n");
	fprintf(f, "\tassert(lx->opaque != NULL);\n");
	fprintf(f, "\n");
	fprintf(f, "\treturn fgetc(lx->opaque);\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");

	fprintf(f, "int\n");
	fprintf(f, "lx_sgetc(struct lx *lx)\n");
	fprintf(f, "{\n");
	fprintf(f, "\tchar *s;\n");
	fprintf(f, "\n");
	fprintf(f, "\tassert(lx != NULL);\n");
	fprintf(f, "\tassert(lx->opaque != NULL);\n");
	fprintf(f, "\n");
	fprintf(f, "\ts = lx->opaque;\n");
	fprintf(f, "\tif (*s == '\\0') {\n");
	fprintf(f, "\t\treturn EOF;\n");
	fprintf(f, "\t}\n");
	fprintf(f, "\n");
	fprintf(f, "\treturn lx->opaque = s + 1, *s;\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");

	fprintf(f, "int\n");
	fprintf(f, "lx_agetc(struct lx *lx)\n");
	fprintf(f, "{\n");
	fprintf(f, "\tstruct lx_arr *a;\n");
	fprintf(f, "\n");
	fprintf(f, "\tassert(lx != NULL);\n");
	fprintf(f, "\tassert(lx->opaque != NULL);\n");
	fprintf(f, "\n");
	fprintf(f, "\ta = lx->opaque;\n");
	fprintf(f, "\n");
	fprintf(f, "\tassert(a != NULL);\n");
	fprintf(f, "\tassert(a->p != NULL);\n");
	fprintf(f, "\n");
	fprintf(f, "\tif (a->len == 0) {\n");
	fprintf(f, "\t\treturn EOF;\n");
	fprintf(f, "\t}\n");
	fprintf(f, "\n");
	fprintf(f, "\treturn a->len--, *a->p++;\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");

	/* TODO: POSIX only */
	fprintf(f, "int\n");
	fprintf(f, "lx_dgetc(struct lx *lx)\n");
	fprintf(f, "{\n");
	fprintf(f, "\tstruct lx_fd *d;\n");
	fprintf(f, "\n");
	fprintf(f, "\tassert(lx != NULL);\n");
	fprintf(f, "\tassert(lx->opaque != NULL);\n");
	fprintf(f, "\n");
	fprintf(f, "\td = lx->opaque;\n");
	fprintf(f, "\tassert(d->fd != -1);\n");
	fprintf(f, "\tassert(d->p != NULL);\n");
	fprintf(f, "\n");
	fprintf(f, "\tif (d->len == 0) {\n");
	fprintf(f, "\t\tssize_t r;\n");
	fprintf(f, "\n");
	fprintf(f, "\t\tassert(fcntl(d->fd, F_GETFL) & O_NONBLOCK == 0);\n");
	fprintf(f, "\n");
	fprintf(f, "\t\td->p = (char *) d + sizeof *d;\n");
	fprintf(f, "\n");
	fprintf(f, "\t\tr = read(d->fd, d->p, d->bufsz);\n");
	fprintf(f, "\t\tif (r == -1) {\n");
	fprintf(f, "\t\t\tassert(errno != EAGAIN);\n");
	fprintf(f, "\t\t\treturn EOF;\n");
	fprintf(f, "\t\t}\n");
	fprintf(f, "\n");
	fprintf(f, "\t\tif (r == 0) {\n");
	fprintf(f, "\t\t\treturn EOF;\n");
	fprintf(f, "\t\t}\n");
	fprintf(f, "\n");
	fprintf(f, "\t\td->len = r;\n");
	fprintf(f, "\t}\n");
	fprintf(f, "\n");
	fprintf(f, "\treturn d->len--, *d->p++;\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");
}

static void
out_io(FILE *f)
{
	/* TODO: consider passing char *c, and return int 0/-1 for error */
	fprintf(f, "static int\n");
	fprintf(f, "lx_getc(struct lx *lx)\n");
	fprintf(f, "{\n");
	fprintf(f, "\tint c;\n");
	fprintf(f, "\n");
	fprintf(f, "\tassert(lx != NULL);\n");
	fprintf(f, "\tassert(lx->lgetc != NULL);\n");
	fprintf(f, "\n");
	fprintf(f, "\tif (lx->c != EOF) {\n");
	fprintf(f, "\t\tc = lx->c, lx->c = EOF;\n");
	fprintf(f, "\t} else {\n");
	fprintf(f, "\t\tc = lx->lgetc(lx);\n");
	fprintf(f, "\t\tif (c == EOF) {\n");
	fprintf(f, "\t\t\treturn EOF;\n");
	fprintf(f, "\t\t}\n");
	fprintf(f, "\t}\n");
	fprintf(f, "\n");
	fprintf(f, "\tif (c == '\\n') {\n");
	fprintf(f, "\t\tlx->line++;\n");
	fprintf(f, "\t}\n");
	fprintf(f, "\n");
	fprintf(f, "\tlx->byte++;\n");
	fprintf(f, "\n");
	fprintf(f, "\tif (lx->push != NULL) {\n");
	fprintf(f, "\t\tif (-1 == lx->push(lx, c)) {\n");
	fprintf(f, "\t\t\treturn EOF;\n");
	fprintf(f, "\t\t}\n");
	fprintf(f, "\t}\n");
	fprintf(f, "\n");
	fprintf(f, "\treturn c;\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");

	fprintf(f, "static void\n");
	fprintf(f, "lx_ungetc(struct lx *lx, int c)\n");
	fprintf(f, "{\n");
	fprintf(f, "\tassert(lx != NULL);\n");
	fprintf(f, "\tassert(lx->c == EOF);\n");
	fprintf(f, "\n");
	fprintf(f, "\tlx->c = c;\n");
	fprintf(f, "\n");
	fprintf(f, "\tif (lx->pop != NULL) {\n");
	fprintf(f, "\t\tlx->pop(lx);\n");
	fprintf(f, "\t}\n");
	fprintf(f, "\n");
	fprintf(f, "\tif (c == '\\n') {\n");
	fprintf(f, "\t\tlx->line--;\n");
	fprintf(f, "\t}\n");
	fprintf(f, "\n");
	fprintf(f, "\tlx->byte--;\n");
	fprintf(f, "}\n");
}

static void
out_zone(FILE *f, const struct ast *ast, const struct ast_zone *z)
{
	assert(f != NULL);
	assert(z != NULL);
	assert(z->fsm != NULL);
	assert(fsm_all(z->fsm, fsm_isdfa));
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
	fprintf(f, "\n");

	assert(z->fsm->start != NULL);
	fprintf(f, "\tstate = S%u;\n", indexof(z->fsm, z->fsm->start));
	fprintf(f, "\n");

	{
		struct fsm_state *s;

		fprintf(f, "\twhile (c = lx_getc(lx), c != EOF) {\n");

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

		fprintf(f, "\tlx->lgetc = NULL;\n");

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
		if (!fsm_all(z->fsm, fsm_isdfa)) {
			errno = EINVAL;
			return;
		}
	}

	fprintf(f, "/* Generated by lx */\n");	/* TODO: date, input etc */
	fprintf(f, "\n");

	fprintf(f, "#include <assert.h>\n");
	fprintf(f, "#include <stdio.h>\n");
	fprintf(f, "#include <errno.h>\n");
	fprintf(f, "\n");

	/* TODO: POSIX only */
	fprintf(f, "#include <unistd.h>\n");
	fprintf(f, "#include <fcntl.h>\n");
	fprintf(f, "\n");

	fprintf(f, "#include LX_HEADER\n");
	fprintf(f, "\n");

	for (z = ast->zl; z != NULL; z = z->next) {
		out_proto(f, ast, z);
	}

	fprintf(f, "\n");

	out_io(f);
	out_lgetc(f);
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
		fprintf(f, "lx_next(struct lx *lx)\n");
		fprintf(f, "{\n");

		fprintf(f, "\tenum lx_token t;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\n");

		fprintf(f, "\tlx->c = EOF;\n");
		fprintf(f, "\n");

		fprintf(f, "\tif (lx->lgetc == NULL) {\n");
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

