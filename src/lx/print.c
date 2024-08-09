/*
 * Copyright 2008-2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/print.h>

#include "print.h"

int
lx_print(FILE *f, const struct ast *ast, const struct fsm_options *opt,
	enum lx_print_lang lang)
{
	lx_print_f *print;

	assert(ast != NULL);

	if (lang == LX_PRINT_NONE) {
		return 0;
	}

	switch (lang) {
	case LX_PRINT_C:    print = lx_print_c;    break;
	case LX_PRINT_DOT:  print = lx_print_dot;  break;
	case LX_PRINT_DUMP: print = lx_print_dump; break;
	case LX_PRINT_H:    print = lx_print_h;    break;
	case LX_PRINT_ZDOT: print = lx_print_zdot; break;

	default:
		errno = ENOTSUP;
		return -1;
	}

	// TODO: return int
	print(f, ast, opt);

	return 0;
}

