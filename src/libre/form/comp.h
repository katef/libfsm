/* $Id$ */

#ifndef RE_COMP_H
#define RE_COMP_H

#include <re/re.h>

struct fsm *
comp_literal(int (*f)(void *opaque), void *opaque,
	enum re_cflags cflags, enum re_err *err, unsigned *byte);

struct fsm *
comp_glob(int (*f)(void *opaque), void *opaque,
	enum re_cflags cflags, enum re_err *err, unsigned *byte);

struct fsm *
comp_simple(int (*f)(void *opaque), void *opaque,
	enum re_cflags cflags, enum re_err *err, unsigned *byte);

#endif

