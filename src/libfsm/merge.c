/* $Id$ */

#include <assert.h>
#include <stdlib.h>
#include <limits.h>

#include <fsm/bool.h>

#include "internal.h"

struct fsm *
fsm_merge(struct fsm *a, struct fsm *b)
{
	struct fsm_state **s;

	assert(a != NULL);
	assert(b != NULL);

	for (s = &a->sl; *s != NULL; s = &(*s)->next) {
		/* empty */
	}

	*s = b->sl;

	b->sl = NULL;
	fsm_free(b);

	fsm_setstart(a, NULL);

	return a;
}

