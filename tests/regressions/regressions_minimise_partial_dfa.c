#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <fsm/fsm.h>

/* A partial DFA where two accepting leaf states are equivalent:
 *
 *     0 -a-> 1 (end)
 *     0 -b-> 2 (end)
 *
 * States 1 and 2 have no outgoing edges at all, so the minimal DFA has two
 * states: the start state and one merged leaf.
 *
 * Previously, trying to partition the {1, 2} equivalence class on a label
 * with no edge took the "dead state" path in try_partition() with
 * uninitialized label sets, so label_sets_match() compared stack garbage and
 * (depending on the build) failed to merge the two states. */
int main(void)
{
	struct fsm *fsm = fsm_new(NULL);
	fsm_state_t s;

	assert(fsm != NULL);
	assert(fsm_addstate(fsm, &s) == 1);
	assert(s == 0);
	assert(fsm_addstate(fsm, &s) == 1);
	assert(s == 1);
	assert(fsm_addstate(fsm, &s) == 1);
	assert(s == 2);

	assert(fsm_addedge_literal(fsm, 0, 1, 'a') == 1);
	assert(fsm_addedge_literal(fsm, 0, 2, 'b') == 1);

	fsm_setend(fsm, 1, 1);
	fsm_setend(fsm, 2, 1);
	fsm_setstart(fsm, 0);

	assert(fsm_minimise(fsm) == 1);

	const size_t n = fsm_countstates(fsm);
	if (n != 2) {
		fprintf(stderr, "expected 2 states after minimisation, got %zu\n", n);
		return 1;
	}

	fsm_free(fsm);
	return 0;
}
