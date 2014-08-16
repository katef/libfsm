/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>

#include <re/re.h>

#include "../comp.h"

#include "lexer.h"
#include "parser.h"

static int
lgetc(struct lx *lx)
{
	const struct lex_state *lex_state;

	assert(lx != NULL);
	assert(lx->opaque != NULL);

	lex_state = lx->opaque;

	assert(lex_state->lgetc != NULL);

	return lex_state->lgetc(lex_state->opaque);
}

struct fsm *
comp_literal(int (*f)(void *opaque), void *opaque,
	enum re_cflags cflags, enum re_err *err, unsigned *byte)
{
	struct act_state act_state_s;
	struct act_state *act_state;
	struct lex_state lex_state_s;
	struct lex_state *lex_state;
	struct lx *lx;
	struct fsm *new;
	enum re_err e;

	assert(f != NULL);

	new = re_new_empty();
	if (new == NULL) {
		e = RE_ENOMEM;
		goto error;
	}

	lex_state = &lex_state_s;

	lex_state->p = lex_state->a;

	lx = &lex_state->lx;

	lx_literal_init(lx);

	lx->lgetc  = lgetc;
	lx->opaque = lex_state;
	lex_state->lgetc   = f;
	lex_state->opaque  = opaque;

	lex_state->buf.a   = NULL;
	lex_state->buf.len = 0;

	/* XXX: unneccessary since we're lexing from a string */
	lx->buf   = &lex_state->buf;
	lx->push  = lx_literal_dynpush;
	lx->pop   = lx_literal_dynpop;
	lx->clear = lx_literal_dynclear;
	lx->free  = lx_literal_dynfree;

/* XXX */
lx->free = NULL;

	/* This is a workaround for ADVANCE_LEXER assuming a pointer */
	act_state = &act_state_s;

	act_state->err      = RE_ESUCCESS;
	act_state->lex_next = lx_literal_next;

	ADVANCE_LEXER;
	p_re__literal(new, cflags, lex_state, act_state);

	if (act_state->err != RE_ESUCCESS) {
		/* TODO: free internals allocated during parsing (are there any?) */
		fsm_free(new);
		e = act_state->err;
		goto error;
	}

	return new;

error:

	if (err != NULL) {
		*err = e;
	}

	if (byte != NULL) {
		*byte = lx->start.byte;
	}

	return NULL;
}

