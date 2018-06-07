/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_DIALECT_COMP_H
#define RE_DIALECT_COMP_H

#include <re/re.h>

#include "../re_ast.h"

/* TODO: make overlap a flag */
/* TODO: rename: "comp" is now a misnomer */

typedef struct ast_re *
re_dialect_parse_fun(re_getchar_fun *getchar, void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, int overlap,
	struct re_err *err);

re_dialect_parse_fun parse_re_literal;

#if 0
struct fsm *
comp_like(int (*f)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, int overlap,
	struct re_err *err);

struct fsm *
comp_literal(int (*f)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, int overlap,
	struct re_err *err);

struct fsm *
comp_glob(int (*f)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, int overlap,
	struct re_err *err);

struct fsm *
comp_native(int (*f)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, int overlap,
	struct re_err *err);

struct fsm *
comp_sql(int (*f)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, int overlap,
	struct re_err *err);

struct fsm *
comp_pcre(int (*f)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, int overlap,
	struct re_err *err);
#endif

#endif

