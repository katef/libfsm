/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_INTERNAL_PRINT_H
#define RE_INTERNAL_PRINT_H

#include <stdio.h>

struct ast_re;
struct fsm_options;

typedef void (re_ast_print)(FILE *f, const struct fsm_options *opt,
	const struct ast_re *ast);

re_ast_print re_ast_print_dot;
re_ast_print re_ast_print_abnf;
re_ast_print re_ast_print_pcre;
re_ast_print re_ast_print_tree;

#endif
