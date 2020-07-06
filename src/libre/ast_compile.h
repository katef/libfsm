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

int
ast_compile(const struct ast *ast,
	struct fsm *fsm, fsm_state_t *start,
	enum re_flags re_flags,
	struct re_err *err);

#endif
