/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <re/re.h>
#include <fsm/fsm.h>

#include "../../libre/internal.h"
#include "../comp.h"

#include "lexer.h"
#include "parser.h"

struct re *
comp_simple(const char *s, enum re_err *err)
{
	struct act_state act_state_s;
	struct act_state *act_state;
	struct lex_state *lex_state;
	struct re *new;
	enum re_err e;

	assert(s != NULL);

	new = re_new_empty();
	if (new == NULL) {
		goto error;
	}

	lex_state = lex_init(s);
	if (lex_state == NULL) {
		re_free(new);
		e = RE_ENOMEM;
		goto error;
	}

	/* This is a workaround for ADVANCE_LEXER assuming a pointer */
	act_state = &act_state_s;

	act_state->err = RE_ESUCCESS;

	ADVANCE_LEXER;
	p_re__simple(new, lex_state, act_state);

	lex_free(lex_state);

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

