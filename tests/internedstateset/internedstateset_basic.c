/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include <adt/internedstateset.h>

int main(void) {
	struct interned_state_set *empty;
	struct interned_state_set *cur, *set_up, *set_up2, *set_down, *set_down2;
	fsm_state_t i;
	const size_t limit = 1000;

	struct interned_state_set_pool *issp = interned_state_set_pool_alloc(NULL);
	assert(issp);

	empty = interned_state_set_empty(issp);
	assert(empty != NULL);

	/* Create state sets with [], [0], [0, 1], ... [0, ... limit-1] */
	cur = empty;
	for (i = 0; i < limit; i++) {
		cur = interned_state_set_add(issp, cur, i);
		assert(cur != NULL);
	}
	set_up = cur;

	/* Create the same ones again */
	cur = empty;
	for (i = 0; i < limit; i++) {
		cur = interned_state_set_add(issp, cur, i);
		assert(cur != NULL);
	}
	set_up2 = cur;

	/* Check that they match. */
	assert(set_up == set_up2);

	/* Create state sets [], [limit-1], [limit-2, limit-1], ... */
	cur = empty;
	i = limit;
	while (i > 0) {
		i--;
		cur = interned_state_set_add(issp, cur, i);
		assert(cur != NULL);
	}
	set_down = cur;

	/* Both should end up with the same state set once all are added. */
	assert(set_up == set_down);

	/* Create the state set in descending order again. These should all
	 * be cached. */
	cur = empty;
	i = limit;
	while (i > 0) {
		i--;
		cur = interned_state_set_add(issp, cur, i);
		assert(cur != NULL);
	}
	set_down2 = cur;

	/* These should also be pointer-equal. */
	assert(set_down2 == set_down);

	/* Free the pool. Check that no memory leaked. */
	interned_state_set_pool_free(issp);
	return 0;
}
