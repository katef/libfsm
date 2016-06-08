/* $Id$ */

#ifndef RE_DIALECT_GROUP_H
#define RE_DIALECT_GROUP_H

#include <re/re.h>

int
group_simple(int (*f)(void *opaque), void *opaque,
	enum re_flags flags, struct re_err *err, struct re_grp *g);

#endif

