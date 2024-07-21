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

	fsm_end_id_t all_ids[2];
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
	memset(all_ids, 0, sizeof all_ids);
	for (state_ind = 0; state_ind < nstates; state_ind++) {
		if (fsm_isend(comb, state_ind)) {
			fsm_end_id_t ids[3] = {0,0,0};
			size_t count;
			int ret;

			count = fsm_endid_count(comb, state_ind);
			printf("state %u count = %zu\n", state_ind, count);
			assert(count == 3);

			ret = fsm_endid_get(comb, state_ind, count, &ids[0]);
			assert(ret == 1);

			assert(ids[0] == 1 && ids[1] == 2 && ids[2] == 4);
		}
	}

	/* execute the fsm and make sure that the right end states correspond to the
	 * right endids
	 */

        fsm_free(comb);

        printf("ok\n");

	return EXIT_SUCCESS;
}


