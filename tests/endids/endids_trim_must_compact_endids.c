#include <stdlib.h>
#include <stdio.h>

#include <assert.h>

#include <fsm/fsm.h>
#include <fsm/print.h>

/* Regression: When fsm_trim called fsm_compact_states it previously
 * didn't remap endids, so the endid collection could refer to stale
 * state IDs or past the end of fsm->states[]. */
int main(void)
{
	size_t limit = 100;
	const fsm_end_id_t end_id = 12345;

	/* Create FSMs with a bunch of garbage states, to increase
	 * the likelihood that the endid's state not being updated
	 * after calling fsm_trim triggers the bug. */
	for (size_t end_state = 1; end_state < limit; end_state++) {
		struct fsm *fsm = fsm_new(NULL);
		assert(fsm != NULL);

		for (size_t i = 0; i < limit; i++) {
			if (!fsm_addstate(fsm, NULL)) {
				return EXIT_FAILURE;
			}
		}

		const fsm_state_t start_state = end_state - 1;
		fsm_setstart(fsm, start_state);
		fsm_setend(fsm, end_state, 1);

		/* add an any edge, so the end state is reachable */
		if (!fsm_addedge_any(fsm, start_state, end_state)) {
			return EXIT_FAILURE;
		}

		if (!fsm_setendid(fsm, end_id)) {
			return EXIT_FAILURE;
		}

		/* Call fsm_trim, as fsm_minimise would. */
		(void)fsm_trim(fsm, FSM_TRIM_START_AND_END_REACHABLE, NULL);

		/* Execute the fsm and check that the endid's state was updated. */
		fsm_state_t end;
		const char *input = "x";
		int ret = fsm_exec(fsm, fsm_sgetc, &input, &end, NULL);
		assert(ret == 1);

		size_t count = fsm_endid_count(fsm, end);
		assert(count == 1);

		fsm_end_id_t id_buf[1];
		if (!fsm_endid_get(fsm, end, 1, id_buf)) {
			return EXIT_FAILURE;
		}
		assert(id_buf[0] == end_id);

		fsm_free(fsm);
	}
	return EXIT_SUCCESS;
}
