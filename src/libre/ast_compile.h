/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_AST_COMPILE_H
#define RE_AST_COMPILE_H

/*
 * Compilation to FSM
 */

struct fsm;
struct re_err;

struct fsm *
ast_compile(const struct ast *ast,
	enum re_flags re_flags,
	const struct fsm_options *opt,
	struct re_err *err);

#endif
