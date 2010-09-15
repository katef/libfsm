/* $Id: comp.c 2 2009-03-08 17:12:37Z kate $ */

#include <assert.h>
#include <stddef.h>

#include <re/re.h>
#include <fsm/fsm.h>

#include "../../libre/internal.h"
#include "../comp.h"

#include "lexer.h"
#include "parser.h"

struct re *
comp_glob(int (*f)(void *opaque), void *opaque, enum re_err *err)
{
	struct act_state act_state_s;
	struct act_state *act_state;
	struct lex_state *lex_state;
	struct re *new;
	enum re_err e;

	assert(f != NULL);

	new = re_new_empty();
	if (new == NULL) {
		e = RE_ENOMEM;
		goto error;
	}

	lex_state = lex_glob_init(f, opaque);
	if (lex_state == NULL) {
		re_free(new);
		e = RE_ENOMEM;
		goto error;
	}

	/* This is a workaround for ADVANCE_LEXER assuming a pointer */
	act_state = &act_state_s;

	act_state->err           = RE_ESUCCESS;
	act_state->lex_nexttoken = lex_glob_nexttoken;
	act_state->lex_tokval    = lex_glob_tokval;

	ADVANCE_LEXER;
	p_re__glob(new, lex_state, act_state);

	/* TODO: no need to malloc lex_state; could use automatic storage */
	lex_glob_free(lex_state);

	if (act_state->err != RE_ESUCCESS) {
		/* TODO: free internals allocated during parsing (are there any?) */
		re_free(new);
		e = act_state->err;
		goto error;
	}

	return new;

error:

	if (err != NULL) {
		*err = e;
	}

	return NULL;
}

