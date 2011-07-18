/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

static int lx_getc(void *opaque) {
	assert(opaque != NULL);

	return fgetc(opaque);
}

static void lx_ungetc(int c, void *opaque) {
	assert(opaque != NULL);
	assert(c >= 0);

	if (EOF == ungetc(c, opaque)) {
		perror("ungetc");
		exit(1);
	}
}

int main(void) {
	enum lx_token t;
	struct lx lx = { 0 };

	lx.getc   = lx_getc;
	lx.ungetc = lx_ungetc;
	lx.opaque = stdin;

	do {

		t = lx_nexttoken(&lx);

		printf("<%s>\n", lx_name(t));

	} while (t != TOK_ERROR && t != TOK_EOF);

	return t == TOK_ERROR;
}

