/* $Id$ */

/*
 * TODO: {counts} (produce unsigned long)
 * TODO: \escapes
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

struct lex_state {
	const char *input;
	char c;
};

struct lex_state *
lex_init(const char *s)
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
lex_free(struct lex_state *state)
{
	assert(state != NULL);

	free(state);
}

enum lex_tok
lex_nexttoken(struct lex_state *state)
{
	assert(state != NULL);
	assert(state->input != NULL);

	if (*state->input == '\0') {
		return TOK_EOF;
	}

	state->c = *state->input;
	state->input++;

	switch (state->c) {
	case '^': return TOK_SOL;
	case '$': return TOK_EOL;
	case '?': return TOK_QMARK;
	case '*': return TOK_STAR;
	case '+': return TOK_PLUS;
	case '.': return TOK_DOT;
	case '|': return TOK_ALT;
	case '-': return TOK_SEP;
	case '(': return TOK_OPEN__SUB;   case ')': return TOK_CLOSE__SUB;
	case '[': return TOK_OPEN__GROUP; case ']': return TOK_CLOSE__GROUP;
	default:  return TOK_CHAR;
	}

	assert(!"unreached");
	return TOK_EOF;
}

char
lex_tokval(struct lex_state *state)
{
	assert(state != NULL);

	return state->c;
}

