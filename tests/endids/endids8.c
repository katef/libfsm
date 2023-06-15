#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include <assert.h>

#include <re/re.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/walk.h>

#include "endids_utils.h"

/* test that intersections of duplicate endids will have states with no duplicates */
int main(void)
{
	struct fsm *fsm1, *fsm2, *comb;
	const char *s1, *s2;
	int ret;

	fsm_end_id_t all_endids[2];
	unsigned nstates, state_ind;

	s1 = "abc"; fsm1 = re_comp(RE_NATIVE, fsm_sgetc, &s1, NULL, 0, NULL);
	s2 = "def"; fsm2 = re_comp(RE_NATIVE, fsm_sgetc, &s2, NULL, 0, NULL);

	assert(s1 != NULL);
	assert(s2 != NULL);

        ret = fsm_determinise(fsm1);
        assert(ret == 1);

        ret = fsm_minimise(fsm1);
        assert(ret == 1);

	ret = fsm_setendid(fsm1, (fsm_end_id_t) 1);
	assert(ret == 1);

	ret = fsm_setendid(fsm1, (fsm_end_id_t) 4);
	assert(ret == 1);


        ret = fsm_determinise(fsm2);
        assert(ret == 1);

        ret = fsm_minimise(fsm2);
        assert(ret == 1);

	ret = fsm_setendid(fsm2, (fsm_end_id_t) 2);
	assert(ret == 1);

	ret = fsm_setendid(fsm2, (fsm_end_id_t) 1);
	assert(ret == 1);


	comb = fsm_intersect(fsm1, fsm2);
	assert(comb != NULL);

        assert( fsm_all(comb, fsm_isdfa) );
        assert( !fsm_empty(comb) );

	// find end states, make sure we have two end states and they each have endids
	nstates = fsm_countstates(comb);

        // all end states should have BOTH endids
	memset(all_endids, 0, sizeof all_endids);
	for (state_ind = 0; state_ind < nstates; state_ind++) {
		if (fsm_isend(comb, state_ind)) {
			fsm_end_id_t endids[3] = {0,0,0};
			size_t nwritten;
			size_t num_endids;
			enum fsm_getendids_res ret;

			nwritten = 0;
			num_endids = fsm_getendidcount(comb, state_ind);

			printf("state %u num_endids = %zu\n", state_ind, num_endids);

			assert(num_endids == 3);

			ret = fsm_getendids(
				comb,
				state_ind,
				sizeof endids / sizeof endids[0],
				&endids[0],
				&nwritten);

			assert(ret == FSM_GETENDIDS_FOUND);
			assert(nwritten == num_endids);

                        qsort(&endids[0], num_endids, sizeof endids[0], cmp_endids);
                        assert(endids[0] == 1 && endids[1] == 2 && endids[2] == 4);
		}
	}

	/* execute the fsm and make sure that the right end states correspond to the
	 * right endids
	 */

        fsm_free(comb);

        printf("ok\n");

	return EXIT_SUCCESS;
}


