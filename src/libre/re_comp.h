/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_COMP_H
#define RE_COMP_H

#include "re_ast.h"

struct ast_re *
re_parse(enum re_dialect dialect, int (*getc)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, struct re_err *err);

struct fsm *
re_comp_ast(struct ast_re *ast,
    enum re_flags flags,
    const struct fsm_options *opt);

#endif
