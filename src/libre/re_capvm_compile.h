/*
 * Copyright 2022 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_CAPVM_COMPILE_H
#define RE_CAPVM_COMPILE_H

/* The part of the capture VM interface that belongs in
 * libre rather than libfsm, mostly related to compiling
 * a libre AST into a capvm_program. */

#include <stdio.h>

#include "ast.h"
#include <fsm/alloc.h>

struct capvm_program;

enum re_capvm_compile_ast_res {
	RE_CAPVM_COMPILE_AST_OK,
	RE_CAPVM_COMPILE_AST_ERROR_ALLOC = -1,
};

enum re_capvm_compile_ast_res
re_capvm_compile_ast(const struct fsm_alloc *alloc,
	const struct ast *ast,
	enum re_flags re_flags,
	struct capvm_program **program);

#endif
