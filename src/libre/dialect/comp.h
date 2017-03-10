#ifndef RE_DIALECT_COMP_H
#define RE_DIALECT_COMP_H

#include <re/re.h>

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

#endif

