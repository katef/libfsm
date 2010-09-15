/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

struct lex_state {
	int (*getc)(void *opaque);
	void *opaque;
	char c;
};

struct lex_state *
lex_literal_init(int (*getc)(void *opaque), void *opaque)
{
	struct lex_state *new;

	assert(getc != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->getc   = getc;
	new->opaque = opaque;

	return new;
}

void
lex_literal_free(struct lex_state *state)
{
	assert(state != NULL);

	free(state);
}

enum lex_tok
lex_literal_nexttoken(struct lex_state *state)
{
	int c;

	assert(state != NULL);
	assert(state->getc != NULL);

	c = state->getc(state->opaque);
	if (c == EOF) {
		return TOK_EOF;
	}

	state->c = c;

	return TOK_CHAR;
}

char
lex_literal_tokval(struct lex_state *state)
{
	assert(state != NULL);

	return state->c;
}

