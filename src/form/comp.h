/* $Id$ */

#ifndef RE_COMP_H
#define RE_COMP_H

#include <re/re.h>

struct re *
comp_literal(const char *s, enum re_err *err);

struct re *
comp_glob(const char *s, enum re_err *err);

struct re *
comp_simple(const char *s, enum re_err *err);

#endif

