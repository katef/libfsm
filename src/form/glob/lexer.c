/* $Id: lexer.c 2 2009-03-08 17:12:37Z kate $ */

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
lex_glob_init(int (*getc)(void *opaque), void *opaque)
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
lex_glob_free(struct lex_state *state)
{
	assert(state != NULL);

	free(state);
}

enum lex_tok
lex_glob_nexttoken(struct lex_state *state)
{
	int c;

	assert(state != NULL);
	assert(state->getc != NULL);

	c = state->getc(state->opaque);
	state->c = c;

	switch (c) {
	case EOF: return TOK_EOF;
	case '?': return TOK_QMARK;
	case '*': return TOK_STAR;
	default:  return TOK_CHAR;
	}

	assert(!"unreached");
	return TOK_EOF;
}

char
lex_glob_tokval(struct lex_state *state)
{
	assert(state != NULL);

	return state->c;
}

