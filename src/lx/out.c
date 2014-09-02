/* $Id: out.c 254 2011-05-05 10:07:49Z kate $ */

#include <stdio.h>
#include <assert.h>

#include "internal.h"

#include "out/out.h"

struct prefix prefix = {
	"lx_",
	"TOK_",
	""
};

void
lx_print(struct ast *ast, FILE *f, enum lx_out format)
{
	void (*out)(const struct ast *ast, FILE *f) = NULL;

	assert(ast != NULL);
	assert(f != NULL);

	switch (format) {
	case LX_OUT_C:    out = lx_out_c;   break;
	case LX_OUT_H:    out = lx_out_h;   break;
	case LX_OUT_DOT:  out = lx_out_dot; break;
	case LX_OUT_TEST: out = NULL;       break;
	}

	if (out == NULL) {
		return;
	}

	out(ast, f);
}

