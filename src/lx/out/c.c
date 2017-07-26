/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/out.h>
#include <fsm/options.h>

#include "libfsm/internal.h" /* XXX */
#include "libfsm/out.h" /* XXX */

#include "lx/out.h"
#include "lx/ast.h"

static int
skip(void *dummy, const struct fsm *fsm, const struct fsm_state *state)
{
	struct ast_mapping *m;

	(void) dummy;
	assert(fsm != NULL);
	assert(state != NULL);

	if (!fsm_isend(NULL, fsm, state)) {
		return 1;
	}

	m = state->opaque;

	assert(m != NULL);

	if (m->token == NULL) {
		return 1;
	}

	return 0;
}

/* TODO: centralise */
static int
escputc(int c, FILE *f)
{
	size_t i;

	const struct {
		int c;
		const char *s;
	} a[] = {
		{ '\\', "\\\\" },
		{ '\"', "\\\"" },
		{ '\'', "\\\'" },

		{ '\a', "\\a"  },
		{ '\b', "\\b"  },
		{ '\f', "\\f"  },
		{ '\n', "\\n"  },
		{ '\r', "\\r"  },
		{ '\t', "\\t"  },
		{ '\v', "\\v"  }
	};

	assert(c != FSM_EDGE_EPSILON);
	assert(f != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].c == c) {
			return fputs(a[i].s, f);
		}
	}

	/*
	 * Escaping '/' here is a lazy way to avoid keeping state when
	 * emitting '*', '/', since this is used to output example strings
	 * inside comments.
	 */

	if (!isprint((unsigned char) c) || c == '/') {
		return fprintf(f, "\\x%02x", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

/* TODO: centralise, maybe with callback */
static int
escputs(FILE *f, const char *s)
{
	const char *p;
	int r, n;

	assert(f != NULL);
	assert(s != NULL);

	n = 0;

	for (p = s; *p != '\0'; p++) {
		r = escputc(*p, f);
		if (r == -1) {
			return -1;
		}

		n += r;
	}

	return n;
}

/* TODO: centralise */
static void
esctok(FILE *f, const char *s)
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

	for (s = fsm->sl, i = 0; s != NULL; s = s->next, i++) {
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

	for (z = ast->zl, i = 0; z != NULL; z = z->next, i++) {
		if (z == zone) {
			return i;
		}
	}

	assert(!"unreached");
	return 0;
}

/*
 * Given a token, find one of its accepting states which gives
 * the shortest fsm_example(). This is pretty expensive.
 */
static const struct fsm_state *
shortest_example(const struct fsm *fsm, const struct ast_token *token)
{
	const struct fsm_state *goal;
	struct fsm_state *s;
	int min;

	assert(fsm != NULL);
	assert(token != NULL);

	/*
	 * We're nominating fsm->start to mean the given token was not present
	 * in this FSM; this is on the premise that the start state cannot
	 * accept, because lx does not permit empty regexps.
	 */
	goal = fsm->start;
	min  = INT_MAX;

	for (s = fsm->sl; s != NULL; s = s->next) {
		const struct ast_mapping *m;
		int n;

		m = s->opaque;
		if (m == NULL) {
			continue;
		}

		if (m->token != token) {
			continue;
		}

		n = fsm_example(fsm, s, NULL, 0);
		if (-1 == n) {
			perror("fsm_example");
			return NULL;
		}

		if (n < min) {
			min = n;
			goal = s;
		}
	}

	return goal;
}

static int
leaf(FILE *f, const struct fsm *fsm, const struct fsm_state *state,
	const void *opaque)
{
	const struct ast *ast;
	const struct ast_mapping *m;

	assert(opaque != NULL);

	ast = opaque;

	assert(ast != NULL);

	if (!fsm_isend(NULL, fsm, state)) {
		/* XXX: don't need this if complete */
		switch (opt.io) {
		case FSM_IO_GETC:
			fprintf(f, "lx->lgetc = NULL; ");
			break;

		case FSM_IO_STR:
			fprintf(f, "lx->p = NULL; ");
			break;

		case FSM_IO_PAIR:
			fprintf(f, "lx->p = NULL; ");
			break;
		}

		fprintf(f, "return %sUNKNOWN;", prefix.tok);
		return 0;
	}

	m = state->opaque;

	assert(m != NULL);

	/* XXX: don't need this if complete */
	fprintf(f, "%sungetc(lx, c); ", prefix.api);
	fprintf(f, "return ");
	if (m->to != NULL) {
		fprintf(f, "lx->z = z%u, ", zindexof(ast, m->to));
	}
	if (m->token != NULL) {
		fprintf(f, "%s", prefix.tok);
		esctok(f, m->token->s);
	} else {
		fprintf(f, "lx->z(lx)");
	}
	fprintf(f, ";");

	return 0;
}

static void
out_proto(FILE *f, const struct ast *ast, const struct ast_zone *z)
{
	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);

	fprintf(f, "static enum %stoken z%u(struct %slx *lx);\n",
		prefix.api, zindexof(ast, z), prefix.lx);
}

static void
out_lgetc(FILE *f)
{
	if (api_getc & API_FGETC) {
		if (print_progress) {
			fprintf(stderr, " fgetc");
		}

		fprintf(f, "int\n");
		fprintf(f, "%sfgetc(struct %slx *lx)\n", prefix.api, prefix.lx);
		fprintf(f, "{\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\tassert(lx->opaque != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\treturn fgetc(lx->opaque);\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	if (api_getc & API_SGETC) {
		if (print_progress) {
			fprintf(stderr, " sgetc");
		}

		fprintf(f, "int\n");
		fprintf(f, "%ssgetc(struct %slx *lx)\n", prefix.api, prefix.lx);
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
	}

	if (api_getc & API_AGETC) {
		if (print_progress) {
			fprintf(stderr, " agetc");
		}

		fprintf(f, "int\n");
		fprintf(f, "%sagetc(struct %slx *lx)\n", prefix.api, prefix.lx);
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
	}

	if (api_getc & API_FDGETC) {
		if (print_progress) {
			fprintf(stderr, " fdgetc");
		}

		/* TODO: keeping both .bufz and .len is silly; use powers of two */
		fprintf(f, "int\n");
		fprintf(f, "%sdgetc(struct %slx *lx)\n", prefix.api, prefix.lx);
		fprintf(f, "{\n");
		fprintf(f, "\tstruct lx_fd *d;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\tassert(lx->opaque != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\td = lx->opaque;\n");
		fprintf(f, "\tassert(d->fd != -1);\n");
		fprintf(f, "\tassert(d->buf != NULL);\n");
		fprintf(f, "\tassert(d->p != NULL);\n");
		fprintf(f, "\tassert(d->p >= d->buf);\n");
		fprintf(f, "\tassert(d->p <= d->buf + d->bufsz);\n");
		fprintf(f, "\n");
		fprintf(f, "\tif (d->len == 0) {\n");
		fprintf(f, "\t\tssize_t r;\n");
		fprintf(f, "\n");
		fprintf(f, "\t\tassert((fcntl(d->fd, F_GETFL) & O_NONBLOCK) == 0);\n");
		fprintf(f, "\n");
		fprintf(f, "\t\tr = read(d->fd, d->buf, d->bufsz);\n");
		fprintf(f, "\t\tif (r == -1) {\n");
		fprintf(f, "\t\t\tassert(errno != EAGAIN);\n");
		fprintf(f, "\t\t\treturn EOF;\n");
		fprintf(f, "\t\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\t\tif (r == 0) {\n");
		fprintf(f, "\t\t\treturn EOF;\n");
		fprintf(f, "\t\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\t\td->p   = d->buf;\n");
		fprintf(f, "\t\td->len = r;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\treturn d->len--, *d->p++;\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}
}

static void
out_io(FILE *f)
{
	if (print_progress) {
		fprintf(stderr, " io");
	}

	/* TODO: consider passing char *c, and return int 0/-1 for error */
	fprintf(f, "#if __STDC_VERSION__ >= 199901L\n");
	fprintf(f, "inline\n");
	fprintf(f, "#endif\n");
	fprintf(f, "static int\n");
	fprintf(f, "lx_getc(struct %slx *lx)\n", prefix.lx);
	fprintf(f, "{\n");
	fprintf(f, "\tint c;\n");
	fprintf(f, "\n");

	fprintf(f, "\tassert(lx != NULL);\n");

	switch (opt.io) {
	case FSM_IO_GETC:
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
		break;

	case FSM_IO_STR:
		/*
		 * For FSM_IO_STR we treat '\0' as the end of input,
		 * and so there's no need to distinguish it from EOF.
		 * We return '\0' here to save the assignment.
		 */
		fprintf(f, "\tassert(lx->p != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tc = *lx->p++;\n");
		fprintf(f, "\n");
		break;

	case FSM_IO_PAIR:
		fprintf(f, "\tassert(lx->p != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tif (lx->p == lx->e) {\n");
		fprintf(f, "\t\t\treturn EOF;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\tc = *lx->p++;\n");
		fprintf(f, "\n");
		break;
	}

	if (~api_exclude & API_POS) {
		fprintf(f, "\tlx->end.byte++;\n");
		fprintf(f, "\tlx->end.col++;\n");
		fprintf(f, "\n");
		fprintf(f, "\tif (c == '\\n') {\n");
		fprintf(f, "\t\tlx->end.line++;\n");
		fprintf(f, "\t\tlx->end.col = 1;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
	}
	fprintf(f, "\treturn c;\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");

	fprintf(f, "#if __STDC_VERSION__ >= 199901L\n");
	fprintf(f, "inline\n");
	fprintf(f, "#endif\n");
	fprintf(f, "static void\n");
	fprintf(f, "%sungetc(struct %slx *lx, int c)\n", prefix.api, prefix.lx);
	fprintf(f, "{\n");
	fprintf(f, "\tassert(lx != NULL);\n");

	switch (opt.io) {
	case FSM_IO_GETC:
		fprintf(f, "\tassert(lx->c == EOF);\n");
		fprintf(f, "\n");
		fprintf(f, "\tlx->c = c;\n");
		fprintf(f, "\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "\tassert(lx->p != NULL);\n");
		fprintf(f, "\tassert(*(lx->p - 1) == c);\n");
		fprintf(f, "\n");
		fprintf(f, "\tlx->p--;\n");
		fprintf(f, "\n");
		break;

	case FSM_IO_PAIR:
		fprintf(f, "\tassert(lx->p != NULL);\n");
		fprintf(f, "\tassert(*(lx->p - 1) == c);\n");
		fprintf(f, "\n");
		fprintf(f, "\tlx->p--;\n");
		fprintf(f, "\n");
		break;
	}

	if (~api_exclude & API_BUF) {
		fprintf(f, "\tif (lx->pop != NULL) {\n");
		fprintf(f, "\t\tlx->pop(lx);\n");
		fprintf(f, "\t}\n");
	}
	if (~api_exclude & API_POS) {
		fprintf(f, "\n");
		fprintf(f, "\tlx->end.byte--;\n");
		fprintf(f, "\tlx->end.col--;\n");
		fprintf(f, "\n");
		fprintf(f, "\tif (c == '\\n') {\n");
		fprintf(f, "\t\tlx->end.line--;\n");
		fprintf(f, "\t\tlx->end.col = 0; /* XXX: lost information */\n");
		fprintf(f, "\t}\n");
	}
	fprintf(f, "}\n");
	fprintf(f, "\n");
}

static void
out_buf(FILE *f)
{
	if (api_tokbuf & API_DYNBUF) {
		if (print_progress) {
			fprintf(stderr, " dynbuf");
		}

		fprintf(f, "int\n");
		fprintf(f, "%sdynpush(struct %slx *lx, char c)\n", prefix.api, prefix.lx);
		fprintf(f, "{\n");
		fprintf(f, "\tstruct lx_dynbuf *t;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\tassert(c != EOF);\n");
		fprintf(f, "\n");
		fprintf(f, "\tt = lx->buf;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(t != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tif (t->p == t->a + t->len) {\n");
		fprintf(f, "\t\tsize_t len;\n");
		fprintf(f, "\t\tchar *tmp;\n");
		fprintf(f, "\n");
		fprintf(f, "\t\tif (t->len == 0) {\n");
		fprintf(f, "\t\t\tassert(LX_DYN_LOW > 0);\n");
		fprintf(f, "\t\t\tlen = LX_DYN_LOW;\n");
		fprintf(f, "\t\t} else {\n");
		fprintf(f, "\t\t\tlen = t->len * LX_DYN_FACTOR;\n");
		fprintf(f, "\t\t\tif (len < t->len) {\n");
		fprintf(f, "\t\t\t\terrno = ERANGE;\n");
		fprintf(f, "\t\t\t\treturn -1;\n");
		fprintf(f, "\t\t\t}\n");
		fprintf(f, "\t\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\t\ttmp = realloc(t->a, len);\n");
		fprintf(f, "\t\tif (tmp == NULL) {\n");
		fprintf(f, "\t\t\treturn -1;\n");
		fprintf(f, "\t\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\t\tt->p   = tmp + (t->p - t->a);\n");
		fprintf(f, "\t\tt->a   = tmp;\n");
		fprintf(f, "\t\tt->len = len;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(t->p != NULL);\n");
		fprintf(f, "\tassert(t->a != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\t*t->p++ = c;\n");
		fprintf(f, "\n");
		fprintf(f, "\treturn 0;\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");

		fprintf(f, "void\n");
		fprintf(f, "%sdynpop(struct %slx *lx)\n", prefix.api, prefix.lx);
		fprintf(f, "{\n");
		fprintf(f, "\tstruct lx_dynbuf *t;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tt = lx->buf;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(t != NULL);\n");
		fprintf(f, "\tassert(t->a != NULL);\n");
		fprintf(f, "\tassert(t->p >= t->a);\n");
		fprintf(f, "\n");
		fprintf(f, "\tif (t->p == t->a) {\n");
		fprintf(f, "\t\treturn;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\tt->p--;\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");

		fprintf(f, "int\n");
		fprintf(f, "%sdynclear(struct %slx *lx)\n", prefix.api, prefix.lx);
		fprintf(f, "{\n");
		fprintf(f, "\tstruct lx_dynbuf *t;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tt = lx->buf;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(t != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tif (t->len > LX_DYN_HIGH) {\n");
		fprintf(f, "\t\tsize_t len;\n");
		fprintf(f, "\t\tchar *tmp;\n");
		fprintf(f, "\n");
		fprintf(f, "\t\tlen = t->len / LX_DYN_FACTOR;\n");
		fprintf(f, "\n");
		fprintf(f, "\t\ttmp = realloc(t->a, len);\n");
		fprintf(f, "\t\tif (tmp == NULL) {\n");
		fprintf(f, "\t\t\treturn -1;\n");
		fprintf(f, "\t\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\t\tt->a   = tmp;\n");
		fprintf(f, "\t\tt->len = len;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\tt->p = t->a;\n");
		fprintf(f, "\n");
		fprintf(f, "\treturn 0;\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");

		fprintf(f, "void\n");
		fprintf(f, "%sdynfree(struct %slx *lx)\n", prefix.api, prefix.lx);
		fprintf(f, "{\n");
		fprintf(f, "\tstruct lx_dynbuf *t;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tt = lx->buf;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(t != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tfree(t->a);\n");
		fprintf(f, "}\n");
	}

	if (api_tokbuf & API_FIXEDBUF) {
		if (print_progress) {
			fprintf(stderr, " fixedbuf");
		}

		fprintf(f, "int\n");
		fprintf(f, "%sfixedpush(struct %slx *lx, char c)\n", prefix.api, prefix.lx);
		fprintf(f, "{\n");
		fprintf(f, "\tstruct lx_fixedbuf *t;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\tassert(c != EOF);\n");
		fprintf(f, "\n");
		fprintf(f, "\tt = lx->buf;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(t != NULL);\n");
		fprintf(f, "\tassert(t->p != NULL);\n");
		fprintf(f, "\tassert(t->a != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tif (t->p == t->a + t->len) {\n");
		fprintf(f, "\t\terrno = ENOMEM;\n");
		fprintf(f, "\t\treturn -1;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\t*t->p++ = c;\n");
		fprintf(f, "\n");
		fprintf(f, "\treturn 0;\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");

		fprintf(f, "void\n");
		fprintf(f, "%sfixedpop(struct %slx *lx)\n", prefix.api, prefix.lx);
		fprintf(f, "{\n");
		fprintf(f, "\tstruct lx_fixedbuf *t;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tt = lx->buf;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(t != NULL);\n");
		fprintf(f, "\tassert(t->a != NULL);\n");
		fprintf(f, "\tassert(t->p >= t->a);\n");
		fprintf(f, "\n");
		fprintf(f, "\tif (t->p == t->a) {\n");
		fprintf(f, "\t\treturn;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\tt->p--;\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");

		fprintf(f, "int\n");
		fprintf(f, "%sfixedclear(struct %slx *lx)\n", prefix.api, prefix.lx);
		fprintf(f, "{\n");
		fprintf(f, "\tstruct lx_fixedbuf *t;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tt = lx->buf;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(t != NULL);\n");
		fprintf(f, "\tassert(t->p != NULL);\n");
		fprintf(f, "\tassert(t->a != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tt->p = t->a;\n");
		fprintf(f, "\n");
		fprintf(f, "\treturn 0;\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}
}

static int
out_zone(FILE *f, const struct ast *ast, const struct ast_zone *z)
{
	assert(f != NULL);
	assert(z != NULL);
	assert(z->fsm != NULL);
	assert(fsm_all(z->fsm, NULL, fsm_isdfa));
	assert(ast != NULL);

	/* TODO: prerequisite that the FSM is a DFA */

	fprintf(f, "static enum %stoken\n", prefix.api);
	fprintf(f, "z%u(struct %slx *lx)\n", zindexof(ast, z), prefix.lx);
	fprintf(f, "{\n");
	fprintf(f, "\tint c;\n");
	fprintf(f, "\n");

	fsm_out_stateenum(f, z->fsm, z->fsm->sl);
	fprintf(f, "\n");

	fprintf(f, "\tassert(lx != NULL);\n");
	fprintf(f, "\n");

	if (~api_exclude & API_BUF) {
		fprintf(f, "\tif (lx->clear != NULL) {\n");
		fprintf(f, "\t\tlx->clear(lx);\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
	}

	assert(z->fsm->start != NULL);
	fprintf(f, "\tstate = S%u;\n", indexof(z->fsm, z->fsm->start));
	fprintf(f, "\n");

	if (~api_exclude & API_POS) {
		fprintf(f, "\tlx->start = lx->end;\n");
		fprintf(f, "\n");
	}

	switch (opt.io) {
	case FSM_IO_GETC:
		fprintf(f, "\twhile (c = lx_getc(lx), c != EOF) {\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "\twhile (c = lx_getc(lx), c != '\\0') {\n");
		break;

	case FSM_IO_PAIR:
		fprintf(f, "\twhile (c = lx_getc(lx), c != EOF) {\n");
		break;
	}

	{
		struct fsm_state *s;
		int has_skips;

		has_skips = 0;

		for (s = z->fsm->sl; s != NULL; s = s->next) {
			int r;

			r = fsm_reachableall(z->fsm, s, NULL, skip);
			if (r == -1) {
				return -1;
			}

			if (r) {
				has_skips = 1;
				break;
			}
		}

		/*
		 * An optimisation to avoid pushing to the token buffer, where all
		 * states reachable henceforth skip, rather than emitting a token.
		 */
		if (has_skips) {
			fprintf(f, "\t\tswitch (state) {\n");

			for (s = z->fsm->sl; s != NULL; s = s->next) {
				int r;

				r = fsm_reachableall(z->fsm, s, NULL, skip);
				if (r == -1) {
					return -1;
				}

				if (!r) {
					continue;
				}

				fprintf(f, "\t\tcase S%u:\n", indexof(z->fsm, s));
			}

			fprintf(f, "\t\t\tbreak;\n");
			fprintf(f, "\n");

			fprintf(f, "\t\tdefault:\n");
			if (~api_exclude & API_BUF) {
				fprintf(f, "\t\t\tif (lx->push != NULL) {\n");
				fprintf(f, "\t\t\t\tif (-1 == lx->push(lx, %s)) {\n", opt.cp);
				fprintf(f, "\t\t\t\t\treturn %sERROR;\n", prefix.tok);
				fprintf(f, "\t\t\t\t}\n");
				fprintf(f, "\t\t\t}\n");
			}
			fprintf(f, "\t\t\tbreak;\n");
			fprintf(f, "\n");

			fprintf(f, "\t\t}\n");
			fprintf(f, "\n");
		} else {
			if (~api_exclude & API_BUF) {
				fprintf(f, "\t\tif (lx->push != NULL) {\n");
				fprintf(f, "\t\t\tif (-1 == lx->push(lx, %s)) {\n", opt.cp);
				fprintf(f, "\t\t\t\treturn %sERROR;\n", prefix.tok);
				fprintf(f, "\t\t\t}\n");
				fprintf(f, "\t\t}\n");
			}
			fprintf(f, "\n");
		}
	}

	{
		const struct fsm_options *tmp;
		static const struct fsm_options defaults;
		struct fsm_options opt = defaults;

		tmp = z->fsm->opt;

		opt.fragment    = 1; /* XXX */
		opt.comments    = z->fsm->opt->comments;
		opt.case_ranges = z->fsm->opt->case_ranges;
		opt.leaf        = leaf;
		opt.leaf_opaque = (void *) ast;

		z->fsm->opt = &opt;

		(void) fsm_out_c(z->fsm, f);

		z->fsm->opt = tmp;
	}

	fprintf(f, "\t}\n");

	fprintf(f, "\n");

	{
		struct fsm_state *s;

		switch (opt.io) {
		case FSM_IO_GETC:
			fprintf(f, "\tlx->lgetc = NULL;\n");
			fprintf(f, "\n");
			break;

		case FSM_IO_STR:
			fprintf(f, "\tlx->p = NULL;\n");
			fprintf(f, "\n");
			break;

		case FSM_IO_PAIR:
			fprintf(f, "\tlx->p = NULL;\n");
			fprintf(f, "\n");
			break;
		}

		fprintf(f, "\tswitch (state) {\n");

		for (s = z->fsm->sl; s != NULL; s = s->next) {
			const struct ast_mapping *m;

			if (!fsm_isend(NULL, z->fsm, s)) {
				continue;
			}

			m = s->opaque;

			assert(m != NULL);

			fprintf(f, "\tcase S%u: return ", indexof(z->fsm, s));

			/* note: no point in changing zone here, because getc is now NULL */

			if (m->token == NULL) {
				fprintf(f, "%sEOF;\n", prefix.tok);
			} else {
				/* TODO: maybe make a printf-like little language to simplify this */
				fprintf(f, "%s", prefix.tok);
				esctok(f, m->token->s);
				fprintf(f, ";\n");
			}
		}

		fprintf(f, "\tdefault: errno = EINVAL; return %sERROR;\n", prefix.tok);

		fprintf(f, "\t}\n");
	}

	fprintf(f, "}\n\n");

	return 0;
}

static void
out_name(FILE *f, const struct ast *ast)
{
	struct ast_token *t;

	assert(f != NULL);
	assert(ast != NULL);

	if (print_progress) {
		fprintf(stderr, " name");
	}

	fprintf(f, "const char *\n");
	fprintf(f, "%sname(enum %stoken t)\n", prefix.api, prefix.api);
	fprintf(f, "{\n");

	fprintf(f, "\tswitch (t) {\n");

	for (t = ast->tl; t != NULL; t = t->next) {
		fprintf(f, "\tcase %s", prefix.tok);
		esctok(f, t->s);
		fprintf(f, ": return \"");
		esctok(f, t->s);
		fprintf(f, "\";\n");
	}

	fprintf(f, "\tcase %sEOF:     return \"EOF\";\n", prefix.tok);
	fprintf(f, "\tcase %sERROR:   return \"ERROR\";\n", prefix.tok);
	fprintf(f, "\tcase %sUNKNOWN: return \"UNKNOWN\";\n", prefix.tok);

	fprintf(f, "\tdefault: return \"?\";\n");

	fprintf(f, "\t}\n");

	fprintf(f, "}\n");
	fprintf(f, "\n");
}

static int
out_example(FILE *f, const struct ast *ast)
{
	struct ast_token *t;
	struct ast_zone *z;

	assert(f != NULL);
	assert(ast != NULL);

	if (print_progress) {
		fprintf(stderr, " example");
	}

	fprintf(f, "const char *\n");
	fprintf(f, "%sexample(enum %stoken (*z)(struct %slx *), enum %stoken t)\n",
		prefix.api, prefix.api, prefix.lx, prefix.api);
	fprintf(f, "{\n");

	fprintf(f, "\tassert(z != NULL);\n");

	fprintf(f, "\n");

	for (z = ast->zl; z != NULL; z = z->next) {
		fprintf(f, "\tif (z == z%u) {\n", zindexof(ast, z));
		fprintf(f, "\t\tswitch (t) {\n");

		for (t = ast->tl; t != NULL; t = t->next) {
			const struct fsm_state *s;
			char buf[50]; /* 50 looks reasonable for an on-screen limit */
			int n;

			s = shortest_example(z->fsm, t);
			if (s == NULL) {
				return -1;
			}

			if (s == z->fsm->start) {
				continue;
			}

			n = fsm_example(z->fsm, s, buf, sizeof buf);
			if (-1 == n) {
				perror("fsm_example");
				return -1;
			}

			fprintf(f, "\t\tcase %s", prefix.tok);
			esctok(f, t->s);
			fprintf(f, ": return \"");
			escputs(f, buf);
			fprintf(f, "%s", n >= (int) sizeof buf - 1 ? "..." : "");
			fprintf(f, "\";\n");
		}

		fprintf(f, "\t\tdefault: goto error;\n");

		fprintf(f, "\t\t}\n");

		fprintf(f, "\t}%s\n", z->next ? " else" : "");
	}

	fprintf(f, "\n");
	fprintf(f, "error:\n");
	fprintf(f, "\n");
	fprintf(f, "\terrno = EINVAL;\n");
	fprintf(f, "\treturn NULL;\n");

	fprintf(f, "}\n");
	fprintf(f, "\n");

	return 0;
}

void
lx_out_c(const struct ast *ast, FILE *f)
{
	const struct ast_zone *z;
	unsigned int zn;

	assert(f != NULL);

	switch (opt.io) {
	case FSM_IO_GETC: opt.cp = "c"; break;
	case FSM_IO_STR:  opt.cp = "c"; break;
	case FSM_IO_PAIR: opt.cp = "c"; break;
	}

	for (z = ast->zl; z != NULL; z = z->next) {
		if (!fsm_all(z->fsm, NULL, fsm_isdfa)) {
			errno = EINVAL;
			return;
		}
	}

	fprintf(f, "/* Generated by lx */\n");	/* TODO: date, input etc */
	fprintf(f, "\n");

	fprintf(f, "#include <assert.h>\n");
	fprintf(f, "#include <stdio.h>\n");
	if (api_tokbuf & API_DYNBUF) {
		fprintf(f, "#include <stdlib.h>\n");
	}
	fprintf(f, "#include <errno.h>\n");
	fprintf(f, "\n");

	if (api_getc & API_FDGETC) {
		fprintf(f, "#include <unistd.h>\n");
		fprintf(f, "#include <fcntl.h>\n");
		fprintf(f, "\n");
	}

	fprintf(f, "#include LX_HEADER\n");
	fprintf(f, "\n");

	for (z = ast->zl; z != NULL; z = z->next) {
		out_proto(f, ast, z);
	}

	fprintf(f, "\n");

	out_io(f);
	out_lgetc(f);

	out_buf(f);

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

		if (-1 == out_zone(f, ast, z)) {
			return; /* XXX: handle error */
		}
	}

	if (~api_exclude & API_NAME) {
		out_name(f, ast);
	}

	if (~api_exclude & API_EXAMPLE) {
		if (-1 == out_example(f, ast)) {
			return;
		}
	}

	{
		fprintf(f, "void\n");
		fprintf(f, "%sinit(struct %slx *lx)\n", prefix.api, prefix.lx);
		fprintf(f, "{\n");
		fprintf(f, "\tstatic const struct %slx lx_default;\n", prefix.lx);
		fprintf(f, "\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\t*lx = lx_default;\n");
		fprintf(f, "\n");

		switch (opt.io) {
		case FSM_IO_GETC:
			fprintf(f, "\tlx->c = EOF;\n");
			break;

		case FSM_IO_STR:
			break;

		case FSM_IO_PAIR:
			break;
		}

		fprintf(f, "\tlx->z = z%u;\n", zindexof(ast, ast->global));
		if (~api_exclude & API_POS) {
			fprintf(f, "\n");
			fprintf(f, "\tlx->end.byte = 0;\n");
			fprintf(f, "\tlx->end.line = 1;\n");
			fprintf(f, "\tlx->end.col  = 1;\n");
		}
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	{
		fprintf(f, "enum %stoken\n", prefix.api);
		fprintf(f, "%snext(struct %slx *lx)\n", prefix.api, prefix.lx);
		fprintf(f, "{\n");

		fprintf(f, "\tenum %stoken t;\n", prefix.api);
		fprintf(f, "\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\tassert(lx->z != NULL);\n");
		fprintf(f, "\n");

		switch (opt.io) {
		case FSM_IO_GETC:
			fprintf(f, "\tif (lx->lgetc == NULL) {\n");
			break;

		case FSM_IO_STR:
			fprintf(f, "\tif (lx->p == NULL) {\n");
			break;

		case FSM_IO_PAIR:
			fprintf(f, "\tif (lx->p == NULL) {\n");
			break;
		}

		fprintf(f, "\t\treturn %sEOF;\n", prefix.tok);

		fprintf(f, "\t}\n");
		fprintf(f, "\n");

		fprintf(f, "\tt = lx->z(lx);\n");
		fprintf(f, "\n");

		if (~api_exclude & API_BUF) {
			fprintf(f, "\tif (lx->push != NULL) {\n");
			fprintf(f, "\t\tif (-1 == lx->push(lx, '\\0')) {\n");
			fprintf(f, "\t\t\treturn %sERROR;\n", prefix.tok);
			fprintf(f, "\t\t}\n");
			fprintf(f, "\t}\n");
			fprintf(f, "\n");
		}

		fprintf(f, "\treturn t;\n");

		fprintf(f, "}\n");
	}

	fprintf(f, "\n");
}

