/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

int
main(void)
{
	enum lx_token t;
	struct lx_dynbuf buf;
	struct lx lx;

	buf.a   = NULL;
	buf.len = 0;

	lx_init(&lx);

	lx.lgetc       = lx_fgetc;
	lx.getc_opaque = stdin;

	lx.push  = lx_dynpush;
	lx.clear = lx_dynclear;
	lx.free  = lx_dynfree;
	lx.buf_opaque = &buf;

	do {

		t = lx_next(&lx);

		switch (t) {
		case TOK_EOF:
			break;

		case TOK_STR_ESC:
		case TOK_QUOT_ESC:
			printf("\\%.*s", (int) buf.len, buf.a);
			break;

		default:
			printf("%.*s", (int) buf.len, buf.a);
			break;
		}

	} while (t != TOK_ERROR && t != TOK_EOF);

	fflush(stdout);

	return t == TOK_ERROR;
}

