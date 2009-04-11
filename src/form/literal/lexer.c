/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

struct lex_state {
	const char *input;
	char c;
};

struct lex_state *
lex_literal_init(const char *s)
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
lex_literal_free(struct lex_state *state)
{
	assert(state != NULL);

	free(state);
}

enum lex_tok
lex_literal_nexttoken(struct lex_state *state)
{
	assert(state != NULL);
	assert(state->input != NULL);

	if (*state->input == '\0') {
		return TOK_EOF;
	}

	state->c = *state->input;
	state->input++;

	return TOK_CHAR;
}

char
lex_literal_tokval(struct lex_state *state)
{
	assert(state != NULL);

	return state->c;
}

