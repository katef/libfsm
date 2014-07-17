/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <fsm/pred.h>

#include "../internal.h"

int
fsm_isany(const struct fsm *fsm, const struct fsm_state *state)
{
	assert(fsm != NULL);
	assert(state != NULL);

	(void) fsm;
	(void) state;

	return 1;
}

