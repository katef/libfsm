/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_INTERNAL_PRINT_H
#define RE_INTERNAL_PRINT_H

#include <stdio.h>

struct ast;
struct fsm_options;

typedef void (ast_print)(FILE *f, const struct fsm_options *opt,
	enum re_flags re_flags, const struct ast *ast);

ast_print ast_print_dot;
ast_print ast_print_abnf;
ast_print ast_print_pcre;
ast_print ast_print_tree;

#endif
