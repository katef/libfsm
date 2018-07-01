/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "lexer.h"

int stack[20];
unsigned int n;


static int
pop(void)
{
	if (n == 0) {
		return 0;
	}

	return stack[--n];
}

static void
push(int i)
{
	if (n == sizeof stack / sizeof *stack) {
		fprintf(stderr, "stack overflow\n");
		exit(1);
	}

	stack[n++] = i;
}

int
main(void)
{
	struct lx lx = { 0 };

	lx.lgetc       = lx_fgetc;
	lx.getc_opaque = stdin;

	lx_init(&lx);

	for (;;) {
		switch (lx_next(&lx)) {
		case TOK_PRINT:   printf("%d\n", stack[n]); continue;
		case TOK_EOF:     printf("%d\n", stack[n]); return 0;
		case TOK_ERROR:   fprintf(stderr, "%s\n", strerror(errno)); return 1;
		case TOK_UNKNOWN: fprintf(stderr, "unexpected input\n"); return 1;

		case TOK_NUMBER: push(57); continue;	/* TODO: get spelling */

		case TOK_ADD: push(pop() + pop()); continue;
		case TOK_SUB: push(pop() - pop()); continue;
		case TOK_DIV: push(pop() / pop()); continue;
		case TOK_MUL: push(pop() * pop()); continue;
		}
	}

	printf("%d\n", pop());

	return 0;
}

