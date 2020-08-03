/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/cost.h>

#include "../internal.h"

static int
isother(int c)
{
	(void) c;
	return 1;
}

unsigned
fsm_cost_legible(fsm_state_t from, fsm_state_t to, char c)
{
	size_t i;

	/*
	 * Costs here approximate legibility; in particular needing an
	 * escape sequence is seen as worth avoiding where possible. Add
	 * a base cost for the character class, and then the character
	 * value itself as a tiebreaker, so lower character values have
	 * precedence over higher (e.g. 'a' is preferable to 'z').
	 */

	struct {
		int (*is)(int);
		unsigned cost;
	} a[] = {
		{ islower,  1 },
		{ isupper,  2 },
		{ isdigit,  3 },
		{ ispunct,  4 },
		{ isspace,  5 },
		{ isprint,  8 },
		{ isother, 10 }
	};

	/*
	 * The legibility of edges depends only on the FSM transition letter,
	 * regardless of the "from" or "to" states.
	 */
	(void) from;
	(void) to;

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].is(c)) {
			const unsigned char_class_cost = 1000 * a[i].cost;
			return char_class_cost + (unsigned)c;
		}
	}

	assert(!"unreached");

	return FSM_COST_INFINITY;
}

