/*
 * Copyright 2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>

#include <re/re.h>

#include "print.h"

int
ast_print(FILE *f, const struct ast *ast, const struct fsm_options *opt,
    enum re_flags re_flags, enum ast_print_lang lang)
{
	ast_print_f *print;

	assert(ast != NULL);

	if (lang == AST_PRINT_NONE) {
		return 0;
	}

	switch (lang) {
	case AST_PRINT_ABNF: print = ast_print_abnf; break;
	case AST_PRINT_DOT:  print = ast_print_dot;  break;
	case AST_PRINT_PCRE: print = ast_print_pcre; break;
	case AST_PRINT_TREE: print = ast_print_tree; break;

	default:
		errno = ENOTSUP;
		return -1;
	}

	// TODO: return int
	print(f, opt, re_flags, ast);

	return 0;
}

