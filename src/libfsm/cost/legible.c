/* $Id$ */

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

#include <adt/set.h>

#include <fsm/cost.h>

#include "../internal.h"

static int
isother(int c)
{
	(void) c;
	return 1;
}

unsigned
fsm_cost_legible(const struct fsm_state *from, const struct fsm_state *to, int c)
{
	enum fsm_edge_type type;
	size_t i;

	/*
	 * Costs here approximate legibility; in particular needing an escape
	 * sequence is seen as worth avoiding where possible.
 	 *
	 * Note that although not explicitly encoded here, for the same cost
	 * (and assuming a suitable character set), lower characater values
	 * have precedence over higher (e.g. 'a' is preferable to 'z')
	 * because of the traversal order for edges when searching the graph.
	 */

	struct {
		int (*is)(int);
		unsigned cost;
	} a[] = {
		{ islower,  1 },
		{ isupper,  3 },
		{ isalpha,  4 },
		{ isdigit,  6 },
		{ ispunct,  8 },
		{ isspace, 10 },
		{ isprint, 12 },
		{ isother, 20 }
	};

	/*
	 * The legibility of edges depends only on the FSM transition letter,
	 * regardless of the "from" or "to" states.
	 */
	(void) from;
	(void) to;

	type = c;

	if (type == FSM_EDGE_EPSILON) {
		return 0;
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].is(type)) {
			return a[i].cost;
		}
	}

	assert(!"unreached");

	return FSM_COST_INFINITY;
}

