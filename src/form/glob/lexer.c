/* $Id: lexer.c 2 2009-03-08 17:12:37Z kate $ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

struct lex_state {
	const char *input;
	char c;
};

struct lex_state *
lex_glob_init(const char *s)
{
	struct lex_state *new;

	assert(s != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->input = s;

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
	assert(state != NULL);
	assert(state->input != NULL);

	if (*state->input == '\0') {
		return TOK_EOF;
	}

	state->c = *state->input;
	state->input++;

	switch (state->c) {
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

