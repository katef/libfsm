/* $Id$ */

#ifndef RE_COMP_H
#define RE_COMP_H

#include <re/re.h>

struct re *
comp_literal(int (*f)(void *opaque), void *opaque, enum re_err *err);

struct re *
comp_glob(int (*f)(void *opaque), void *opaque, enum re_err *err);

struct re *
comp_simple(int (*f)(void *opaque), void *opaque, enum re_err *err);

#endif

