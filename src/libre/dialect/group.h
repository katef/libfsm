/* $Id$ */

#ifndef RE_DIALECT_GROUP_H
#define RE_DIALECT_GROUP_H

#include <re/re.h>

int
group_native(int (*f)(void *opaque), void *opaque,
	enum re_flags flags, int overlap,
	struct re_err *err, struct re_grp *g);

#endif

