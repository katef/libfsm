/* $Id$ */

#ifndef RE_DIALECT_COMP_H
#define RE_DIALECT_COMP_H

#include <re/re.h>

struct fsm *
comp_literal(int (*f)(void *opaque), void *opaque,
	enum re_flags flags, struct re_err *err);

struct fsm *
comp_glob(int (*f)(void *opaque), void *opaque,
	enum re_flags flags, struct re_err *err);

struct fsm *
comp_native(int (*f)(void *opaque), void *opaque,
	enum re_flags flags, struct re_err *err);

#endif

