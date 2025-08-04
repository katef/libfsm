/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include <print/esc.h>

#include "lx/lx.h"
#include "lx/ast.h"
#include "lx/print.h"

struct lx_hook_env {
	const struct ast *ast;
	/* Name of variable for the current character of input in the
	 * current scope, which depends on the IO options. */
	const char *cur_char_var;
};


static int
skip(const struct fsm *fsm, fsm_state_t state)
{
	struct ast_mapping *m;

	assert(fsm != NULL);
	assert(state < fsm_countstates(fsm));

	if (!fsm_isend(fsm, state)) {
		return 1;
	}

	m = ast_getendmapping(fsm, state);
	if (LOG()) {
		fprintf(stderr, "fsm_print_cfrag: ast_getendmapping for state %d: %p\n",
		    state, (void *)m);
	}
	assert(m != NULL);

	if (m->token == NULL) {
		return 1;
	}

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
static int
shortest_example(const struct fsm *fsm, const struct ast_token *token,
	fsm_state_t *q)
{
	fsm_state_t goal, i;
	int min;

	assert(fsm != NULL);
	assert(token != NULL);
	assert(q != NULL);

	/*
	 * We're nominating the start state to mean the given token was not present
	 * in this FSM; this is on the premise that the start state cannot
	 * accept, because lx does not permit empty regexps.
	 */
	(void) fsm_getstart(fsm, &goal);
	min = INT_MAX;

	const size_t statecount = fsm_countstates(fsm);
	for (i = 0; i < statecount; i++) {
		const struct ast_mapping *m;
		int n;

		if (!fsm_isend(fsm, i)) {
			continue;
		}

		m = ast_getendmapping(fsm, i);
		if (LOG()) {
			fprintf(stderr, "shortest_example: ast_getendmapping for state %d: %p\n",
			    i, (void *)m);
		}
		if (m == NULL) {
			continue;
		}

		if (m->token != token) {
			continue;
		}

		n = fsm_example(fsm, i, NULL, 0);
		if (-1 == n) {
			perror("fsm_example");
			return 0;
		}

		if (n < min) {
			min = n;
			goal = i;
		}
	}

	*q = goal;
	return 1;
}

static const char *
buf_op_prefix(void)
{
	if (api_tokbuf & API_FIXEDBUF) {
		return "fixed";
	} else if (api_tokbuf & API_DYNBUF) {
		return "dyn";
	} else {
		assert(!"buf is neither fixed nor dyn");
		return NULL;
	}
}

static void
unget_character(FILE *f, bool pop, const char *cur_char_var)
{
	fprintf(f, "%sungetc(lx, %s); ", prefix.api, cur_char_var);
	if (pop && (~api_exclude & API_POS)) {
		fprintf(f, "%s%spop(lx->buf_opaque); ",
		    prefix.api, buf_op_prefix());
	}
}

static int
accept_c(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	const struct ast *ast;
	const struct ast_mapping *m;
	struct lx_hook_env *env = hook_opaque;

	assert(f != NULL);
	assert(opt != NULL);
	assert(state_metadata->end_ids != NULL);
	assert(state_metadata->end_id_count > 0);
	assert(lang_opaque == NULL);
	assert(hook_opaque != NULL);

	ast = env->ast;
	m = ast_getendmappingbyendid(state_metadata->end_ids[0]);

	/* re-sync before new call into zone */
	switch (opt->io) {
	case FSM_IO_GETC:
		break;

	case FSM_IO_STR:
	case FSM_IO_PAIR:
		fprintf(f, "lx->p = p; ");
		break;
	}

	fprintf(f, "return ");
	if (m->to == NULL) {
		if (m->token == NULL) {
			/* If accept-ing here doesn't actually map to a token or
			 * a different zone, then it's stuck in the middle of a
			 * pattern pair like `'//' .. /\n/ -> $nl;` with an EOF,
			 * so tokenization should still fail. */
			fprintf(f, "%sUNKNOWN", prefix.tok);
		} else {
			/* yield a token */
			fprintf(f, "%s", prefix.tok);
			esctok(f, m->token->s);
		}
	} else {
		if (m->token == NULL) {
			/* update to a different zone, then call to it */
			fprintf(f, "lx->z = z%u, lx->z(lx)", zindexof(ast, m->to));
		} else {
			/* update zone, then yield a token */
			fprintf(f, "lx->z = z%u, ", zindexof(ast, m->to));
			fprintf(f, "%s", prefix.tok);
			esctok(f, m->token->s);
		}
	}
	fprintf(f, ";");
	return 0;
}

static int
reject_c(FILE *f, const struct fsm_options *opt,
	const struct fsm_state_metadata *state_metadata,
	void *lang_opaque, void *hook_opaque)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(lang_opaque == NULL);
	assert(hook_opaque != NULL);

	(void) lang_opaque;
	struct lx_hook_env *env = hook_opaque;

	const struct ast_mapping *m = state_metadata != NULL && state_metadata->end_id_count > 0
	    ? ast_getendmappingbyendid(state_metadata->end_ids[0])
	    : NULL;

	/* If there is an AST mapping associated with this end state,
	 * then unget the previous character (in most cases), and
	 * possibly emit its token type and/or new z state. */
	if (m != NULL) {
		const bool has_endids = state_metadata && state_metadata->end_id_count > 0;
		if (m->token == NULL && m->to == NULL && !has_endids) {
			unget_character(f, true, env->cur_char_var);
		} else if (m->token == NULL && m->to == NULL && has_endids) {
			unget_character(f, true, env->cur_char_var);
		} else if (m->token == NULL && m->to != NULL) {
		  	unget_character(f, true, env->cur_char_var);
		} else if (m->token != NULL && m->to == NULL) {
			assert(has_endids);
		  	unget_character(f, true, env->cur_char_var);
		} else if (m->token != NULL && m->to != NULL) {
		  	unget_character(f, true, env->cur_char_var);
		}

		/* re-sync before new call into zone */
		switch (opt->io) {
		case FSM_IO_GETC:
			break;

		case FSM_IO_STR:
		case FSM_IO_PAIR:
			fprintf(f, "lx->p = p; ");
			break;
		}

		fprintf(f, "return ");
		if (m->to != NULL) {
			fprintf(f, "lx->z = z%u, ", zindexof(env->ast, m->to));
		}
		if (m->token != NULL) {
			fprintf(f, "%s", prefix.tok);
			esctok(f, m->token->s);
		} else {
			fprintf(f, "lx->z(lx)");
		}
		fprintf(f, ";");
		return 0;
	} else {
		fprintf(f, "\n\t\t\t\tif (!has_consumed_input) { return %sEOF; }\n", prefix.tok);
		fprintf(f, "\t\t\t\t");
		unget_character(f, false, env->cur_char_var);
	}

	/* XXX: don't need this if complete */
	switch (opt->io) {
	case FSM_IO_GETC:
		fprintf(f, "lx->lgetc = NULL; ");
		break;

	case FSM_IO_STR:
	case FSM_IO_PAIR:
		fprintf(f, "lx->p = NULL; ");
		break;
	}

	fprintf(f, "return %sUNKNOWN;", prefix.tok);
	return 0;
}

static int
advance_c(FILE *f, const struct fsm_options *opt, const char *cur_char_var, void *hook_opaque)
{
	(void)hook_opaque;

	switch (opt->io) {
	case FSM_IO_GETC:
		break;

	case FSM_IO_STR:
	case FSM_IO_PAIR:
		/* When libfsm's generated code advances a character, update
		 * lx's token name buffer and position bookkeeping. */
		fprintf(f, "\t\tif (!%sadvance_end(lx, %s)) { return %sERROR; }\n",
		    prefix.api, cur_char_var, prefix.tok);
		break;
	}
	return 0;
}

static void
print_proto(FILE *f, const struct ast *ast, const struct ast_zone *z)
{
	assert(f != NULL);
	assert(ast != NULL);
	assert(z != NULL);

	fprintf(f, "static enum %stoken z%u(struct %slx *lx);\n",
		prefix.api, zindexof(ast, z), prefix.lx);
}

static void
print_lgetc(FILE *f, const struct fsm_options *opt)
{
	if (api_getc & API_FGETC) {
		if (print_progress) {
			fprintf(stderr, " fgetc");
		}

		if (opt->comments) {
			fprintf(f, "/* Get a character from fgetc and push it to the buffer */\n");
		}
		fprintf(f, "int\n");
		fprintf(f, "%sfgetc(struct %slx *lx)\n", prefix.api, prefix.lx);
		fprintf(f, "{\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\tassert(lx->getc_opaque != NULL);\n");
		fprintf(f, "\n");

		fprintf(f, "\tconst int c = fgetc(lx->getc_opaque);\n");
		fprintf(f, "\tif (c == EOF) {\n");
		fprintf(f, "\t\tlx->c = EOF;\n");
		fprintf(f, "\t\treturn EOF;\n");
		fprintf(f, "\t} else {\n");

		fprintf(f, "\t\treturn c;\n");
		fprintf(f, "\t}\n");

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
		fprintf(f, "\tconst char **s;\n");
		fprintf(f, "\tchar c;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\tassert(lx->getc_opaque != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\ts = lx->getc_opaque;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(s != NULL);\n");
		fprintf(f, "\tassert(*s != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tc = **s;\n");
		fprintf(f, "\n");
		fprintf(f, "\tif (c == '\\0') {\n");
		fprintf(f, "\t\treturn EOF;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\t(*s)++;\n");
		fprintf(f, "\n");
		fprintf(f, "\treturn (unsigned char) c;\n");
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
		fprintf(f, "\tassert(lx->getc_opaque != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\ta = lx->getc_opaque;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(a != NULL);\n");
		fprintf(f, "\tassert(a->p != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tif (a->len == 0) {\n");
		fprintf(f, "\t\treturn EOF;\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\treturn a->len--, (unsigned char) *a->p++;\n");
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
		fprintf(f, "\tassert(lx->getc_opaque != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\td = lx->getc_opaque;\n");
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
print_io(FILE *f, const struct fsm_options *opt)
{
	if (print_progress) {
		fprintf(stderr, " io");
	}

	fprintf(f, "static int\n");
	fprintf(f, "%sadvance_end(struct %slx *lx, int c)\n", prefix.api, prefix.lx);
	fprintf(f, "{\n");
	if (~api_exclude & API_POS) {
		fprintf(f, "\tlx->end.byte++;\n");
		fprintf(f, "\tlx->end.col++;\n");

		fprintf(f, "\tif (c == '\\n') {\n");
		fprintf(f, "\t\tlx->end.line++;\n");
		fprintf(f, "\t\tlx->end.saved_col = lx->end.col - 1;\n");
		fprintf(f, "\t\tlx->end.col = 1;\n");

		if (opt->io == FSM_IO_STR) {   /* ignore terminating '\0' */
			fprintf(f, "\t} else if (c == '\\0') { /* don't count terminating '\\0' */\n");
			fprintf(f, "\t\tlx->end.byte--;\n");
			fprintf(f, "\t\tlx->end.col--;\n");
			fprintf(f, "\t}\n");
		} else {
			fprintf(f, "\t}\n");
		}
	}

	if (~api_exclude & API_BUF) {
		fprintf(f, "\tif (lx->push != NULL) {\n");
		fprintf(f, "\t\tif (-1 == lx->push(lx->buf_opaque, (char)c)) {\n");
		fprintf(f, "\t\t\treturn 0;\n");
		fprintf(f, "\t\t}\n");
		fprintf(f, "\t}\n");
	}

	fprintf(f, "\treturn 1;\n");
	fprintf(f, "}\n");
	fprintf(f, "\n");

	if (opt->io == FSM_IO_GETC) {
		/* TODO: consider passing char *c, and return int 0/-1 for error */
		if (opt->comments) {
			fprintf(f, "/* This wrapper manages one character of lookahead/pushback\n");
			fprintf(f, " * and the line, column, and byte offsets. */\n");
		}
		fprintf(f, "#if __STDC_VERSION__ >= 199901L\n");
		fprintf(f, "inline\n");
		fprintf(f, "#endif\n");
		fprintf(f, "static int\n");
		fprintf(f, "%sgetc(struct %slx *lx)\n", prefix.api, prefix.lx);
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

		/* FIXME: This should distinguish between alloc failure
		 * and EOF, but will require layers of interface changes. */
		fprintf(f, "\tif (!%sadvance_end(lx, c)) { return EOF; }\n", prefix.api);
		fprintf(f, "\n");

		fprintf(f, "\treturn c;\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");

		/* Add an implementation of fsm_getc that calls back
		 * into lx_getc with the lx handle. */
		fprintf(f, "/* This wrapper adapts calling %sgetc to the interface\n", prefix.api);
		fprintf(f, " * in libfsm's generated code. */\n");
		fprintf(f, "static int\n");
		fprintf(f, "fsm_getc(void *getc_opaque)\n");
		fprintf(f, "{\n");

		fprintf(f, "\treturn %sgetc((struct %slx *)getc_opaque);\n", prefix.api, prefix.lx);
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	fprintf(f, "#if __STDC_VERSION__ >= 199901L\n");
	fprintf(f, "inline\n");
	fprintf(f, "#endif\n");
	fprintf(f, "static void\n");
	fprintf(f, "%sungetc(struct %slx *lx, int c)\n", prefix.api, prefix.lx);
	fprintf(f, "{\n");
	fprintf(f, "\tassert(lx != NULL);\n");

	switch (opt->io) {
	case FSM_IO_GETC:
		fprintf(f, "\tassert(lx->c == EOF);\n");
		fprintf(f, "\tlx->c = c;\n");
		break;

	case FSM_IO_STR:
		fprintf(f, "\tassert(lx->p != NULL);\n");
		break;

	case FSM_IO_PAIR:
		fprintf(f, "\tassert(lx->p != NULL);\n");
		break;
	}

	if (~api_exclude & API_POS) {
		fprintf(f, "\tlx->end.byte--;\n");
		fprintf(f, "\tlx->end.col--;\n");
		fprintf(f, "\n");
		fprintf(f, "\tif (c == '\\n') {\n");
		fprintf(f, "\t\tlx->end.line--;\n");
		fprintf(f, "\t\tlx->end.col = lx->end.saved_col;\n");
		fprintf(f, "\t}\n");
	}
	fprintf(f, "}\n");
	fprintf(f, "\n");
}

static void
print_buf(FILE *f, const struct fsm_options *opt)
{
	if (api_tokbuf & API_DYNBUF) {
		if (print_progress) {
			fprintf(stderr, " dynbuf");
		}

		fprintf(f, "int\n");
		fprintf(f, "%sdynpush(void *buf_opaque, char c)\n", prefix.api);
		fprintf(f, "{\n");
		fprintf(f, "\tstruct lx_dynbuf *t = buf_opaque;\n");
		fprintf(f, "\n");
		fprintf(f, "\tassert(t != NULL);\n");
		fprintf(f, "\n");
		fprintf(f, "\tif (t->a == NULL || t->p == t->a + t->len) {\n"); /* (t->a == NULL || ...) is to appease ubsan */
		fprintf(f, "\t\tsize_t len;\n");
		fprintf(f, "\t\tptrdiff_t off;\n");
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
		fprintf(f, "\t\toff = t->p - t->a;\n");
		fprintf(f, "\t\ttmp = realloc(t->a, len);\n");
		fprintf(f, "\t\tif (tmp == NULL) {\n");
		fprintf(f, "\t\t\treturn -1;\n");
		fprintf(f, "\t\t}\n");
		fprintf(f, "\n");
		fprintf(f, "\t\tt->p   = tmp + off;\n");
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


		if ((~api_exclude & API_BUF) && (api_tokbuf & API_DYNBUF)) {
			fprintf(f, "static void\n");
			fprintf(f, "%sdynpop(void *buf_opaque)\n", prefix.api);
			fprintf(f, "{\n");
			fprintf(f, "\tstruct lx_dynbuf *t = buf_opaque;\n");
			fprintf(f, "\n");
			fprintf(f, "\tassert(t != NULL);\n");
			fprintf(f, "\n");

			if (opt->io == FSM_IO_GETC) {
				fprintf(f, "\tassert(t->p != t->a);\n");
			}

			fprintf(f, "\tt->p--;\n");

			fprintf(f, "}\n");
			fprintf(f, "\n");
		}

		fprintf(f, "int\n");
		/* FIXME: handle error from dynclear */
		fprintf(f, "%sdynclear(void *buf_opaque)\n", prefix.api);
		fprintf(f, "{\n");
		fprintf(f, "\tstruct lx_dynbuf *t = buf_opaque;\n");
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
		fprintf(f, "%sdynfree(void *buf_opaque)\n", prefix.api);
		fprintf(f, "{\n");
		fprintf(f, "\tstruct lx_dynbuf *t = buf_opaque;\n");
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
		fprintf(f, "%sfixedpush(void *buf_opaque, char c)\n", prefix.api);
		fprintf(f, "{\n");
		fprintf(f, "\tstruct lx_fixedbuf *t = buf_opaque;\n");
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

		if (~api_exclude & API_BUF && (api_tokbuf & API_FIXEDBUF)) {
			fprintf(f, "static void\n");
			fprintf(f, "%sfixedpop(void *buf_opaque)\n", prefix.api);
			fprintf(f, "{\n");
			fprintf(f, "\tstruct lx_fixedbuf *t = buf_opaque;\n");
			fprintf(f, "\n");
			fprintf(f, "\tassert(t != NULL);\n");
			fprintf(f, "\tassert(t->p != NULL);\n");
			fprintf(f, "\tassert(t->a != NULL);\n");

			if (opt->io == FSM_IO_GETC) {
				fprintf(f, "\tassert(t->p > t->a);\n");
			}
			fprintf(f, "\tt->p--;\n");
			fprintf(f, "}\n");
			fprintf(f, "\n");
		}

		fprintf(f, "int\n");
		fprintf(f, "%sfixedclear(void *buf_opaque)\n", prefix.api);
		fprintf(f, "{\n");
		fprintf(f, "\tstruct lx_fixedbuf *t = buf_opaque;\n");
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
print_zone(FILE *f, const struct ast *ast, const struct ast_zone *z,
	const struct fsm_options *opt, const char *cur_char_var)
{
	assert(f != NULL);
	assert(z != NULL);
	assert(z->fsm != NULL);
	assert(fsm_all(z->fsm, fsm_isdfa));
	assert(ast != NULL);
	assert(cur_char_var != NULL);

	/* prerequisite that the FSM is a DFA */
	assert(fsm_all(z->fsm, fsm_isdfa));

	fprintf(f, "static enum %stoken\n", prefix.api);
	fprintf(f, "z%u(struct %slx *lx)\n", zindexof(ast, z), prefix.lx);
	fprintf(f, "{\n");

	switch (opt->io) {
	case FSM_IO_GETC:
		fprintf(f, "\tint c;\n");
		break;
	case FSM_IO_STR:
	case FSM_IO_PAIR:
		break;
	}

	fprintf(f, "\n");

	fprintf(f, "\tassert(lx != NULL);\n");
	fprintf(f, "\n");

	if (~api_exclude & API_BUF) {
		fprintf(f, "\tif (lx->clear != NULL) {\n");
		fprintf(f, "\t\tlx->clear(lx->buf_opaque);\n");
		fprintf(f, "\t}\n");
		fprintf(f, "\n");
	}

	if (~api_exclude & API_POS) {
		fprintf(f, "\tlx->start = lx->end;\n");
		fprintf(f, "\n");
	}

	switch (opt->io) {
	case FSM_IO_GETC:
		fprintf(f, "\tvoid *getc_opaque = (void *)lx;\n");

		/* This must be called with fragment sent, otherwise
		 * it will generate a nested function definition. */
		assert(opt->fragment);
		break;

	case FSM_IO_STR:
		fprintf(f, "const char *s = lx->p;\n");
		fprintf(f, "const char *p;\n");
		break;

	case FSM_IO_PAIR:
		fprintf(f, "\tconst char *p, *b = lx->p, *e = lx->e;\n");
		break;
	}

	{
		static const struct fsm_hooks defaults;
		struct fsm_hooks hooks = defaults;

		struct lx_hook_env hook_env = {
			.ast = ast,
			.cur_char_var = cur_char_var,
		};

		hooks.accept      = accept_c;
		hooks.reject      = reject_c;
		hooks.advance     = advance_c;
		hooks.hook_opaque = &hook_env;

		fsm_print(f, z->fsm, opt, &hooks, FSM_PRINT_C);
	}

	if (~api_exclude & API_BUF) {
		int has_skips;
		fsm_state_t i;

		has_skips = 0;

		const size_t statecount = fsm_countstates(z->fsm);
		for (i = 0; i < statecount; i++) {
			int r;

			r = fsm_reachableall(z->fsm, i, skip);
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
			fprintf(f, "\n");
			fprintf(f, "\t\tswitch (state) {\n");

			for (i = 0; i < statecount; i++) {
				int r;

				r = fsm_reachableall(z->fsm, i, skip);
				if (r == -1) {
					return -1;
				}

				if (!r) {
					continue;
				}

				fprintf(f, "\t\tcase S%u:\n", (unsigned) i);
			}

			fprintf(f, "\t\t\tbreak;\n");
			fprintf(f, "\n");

			fprintf(f, "\t\tdefault:\n");
			fprintf(f, "\t\t\tif (lx->push != NULL) {\n");
			fprintf(f, "\t\t\t\tif (-1 == lx->push(lx->buf_opaque, (char)%s)) {\n", cur_char_var);
			fprintf(f, "\t\t\t\t\treturn %sERROR;\n", prefix.tok);
			fprintf(f, "\t\t\t\t}\n");
			fprintf(f, "\t\t\t}\n");
			fprintf(f, "\t\t\tbreak;\n");
			fprintf(f, "\n");

			fprintf(f, "\t\t}\n");
		} else {
			fprintf(f, "\n");
			fprintf(f, "\t\tif (lx->push != NULL) {\n");
			fprintf(f, "\t\t\tif (-1 == lx->push(lx->buf_opaque, (char)%s)) {\n", cur_char_var);
			fprintf(f, "\t\t\t\treturn %sERROR;\n", prefix.tok);
			fprintf(f, "\t\t\t}\n");
			fprintf(f, "\t\t}\n");
		}
	}

	fprintf(f, "\n");

	{
		switch (opt->io) {
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

		fprintf(f, "\tif (!has_consumed_input) {\n");
		fprintf(f, "\t\treturn %sEOF;\n", prefix.tok);
		fprintf(f, "\t} \n");
		fprintf(f, "\treturn %sERROR;", prefix.tok);
		fprintf(f, "\n");
	}

	fprintf(f, "}\n\n");

	return 0;
}

static void
print_name(FILE *f, const struct ast *ast)
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
print_example(FILE *f, const struct ast *ast, const struct fsm_options *opt)
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
			fsm_state_t s;
			char buf[50]; /* 50 looks reasonable for an on-screen limit */
			int n;

			if (!shortest_example(z->fsm, t, &s)) {
				return -1;
			}

			if (!fsm_getstart(z->fsm, &s)) {
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
			escputs(f, opt, c_escputc_str, buf);
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
lx_print_c(FILE *f, const struct ast *ast, const struct fsm_options *opt)
{
	const char *cp;
	const struct ast_zone *z;
	unsigned int zn;

	assert(f != NULL);
	assert(ast != NULL);
	assert(opt != NULL);

	switch (opt->io) {
	case FSM_IO_GETC: cp = "c"; break;
	case FSM_IO_STR:  cp = "*p"; break;
	case FSM_IO_PAIR: cp = "*p"; break;
	}

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
	if (api_tokbuf & API_DYNBUF) {
		fprintf(f, "#include <stddef.h>\n");
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
		print_proto(f, ast, z);
	}

	fprintf(f, "\n");

	print_io(f, opt);
	print_lgetc(f, opt);

	print_buf(f, opt);

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

		if (-1 == print_zone(f, ast, z, opt, cp)) {
			return; /* XXX: handle error */
		}
	}

	if (~api_exclude & API_NAME) {
		print_name(f, ast);
	}

	if (~api_exclude & API_EXAMPLE) {
		if (-1 == print_example(f, ast, opt)) {
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

		if (opt->io == FSM_IO_GETC) {
			fprintf(f, "\tlx->c = EOF;\n");
		}

		fprintf(f, "\tlx->z = z%u;\n", zindexof(ast, ast->global));
		if (~api_exclude & API_POS) {
			fprintf(f, "\n");
			fprintf(f, "\tlx->end.byte = 0;\n");
			fprintf(f, "\tlx->end.line = 1;\n");
			fprintf(f, "\tlx->end.col  = 1;\n");
		}

		/* Suppress warning for possibly unused function */
		if (~api_exclude & API_BUF) {
			if (api_tokbuf & API_FIXEDBUF) {
				fprintf(f, "\t(void)%sfixedpop;\n", prefix.api);
			} else if (api_tokbuf & API_DYNBUF) {
				fprintf(f, "\t(void)%sdynpop;\n", prefix.api);
			}
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

		switch (opt->io) {
		case FSM_IO_GETC:
			fprintf(f, "\tif (lx->lgetc == NULL) {\n");
			break;

		case FSM_IO_STR:
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
			fprintf(f, "\t\tif (-1 == lx->push(lx->buf_opaque, '\\0')) {\n");
			fprintf(f, "\t\t\treturn %sERROR;\n", prefix.tok);
			fprintf(f, "\t\t}\n");
			fprintf(f, "\t}\n");
			fprintf(f, "\n");
		}

		fprintf(f, "\treturn t;\n");

		fprintf(f, "}\n");
	}

	fprintf(f, "\n");

	if (opt->io == FSM_IO_STR) {
		fprintf(f, "void\n");
		fprintf(f, "%sinput_str(struct %slx *lx, const char *p)\n", prefix.api, prefix.lx);
		fprintf(f, "{\n");
		fprintf(f, "\tassert(lx != NULL);\n");
		fprintf(f, "\tassert(p != NULL);\n");
		if (~api_exclude & API_POS) {
			fprintf(f, "\tlx->end.col = 1;\n");
		}
		fprintf(f, "\tlx->p = p;\n");
		fprintf(f, "}\n");
	}
}
