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

struct state_info {
	fsm_state_t state;
	unsigned num_endids;
	fsm_end_id_t endids[2];
};

/* use fsm_increndids to change the endids before a union/intersection */

void
fsm_increndids(struct fsm * fsm, int delta);

/* test that we can increment endids */
int main(void)
{
	struct fsm *fsm;
	const char *s;
	struct state_info *info;
	size_t nstates, ninfo, state_ind, nend;
	int ret;

	s = "abc"; fsm = re_comp(RE_NATIVE, fsm_sgetc, &s, NULL, 0, NULL);

	ret = fsm_setendid(fsm, (fsm_end_id_t) 1);
	assert(ret == 1);

	{
		struct fsm *fsm2;
		s = "def"; fsm2 = re_comp(RE_NATIVE, fsm_sgetc, &s, NULL, 0, NULL);

		ret = fsm_setendid(fsm2, (fsm_end_id_t) 2);
		assert(ret == 1);

		fsm = fsm_union(fsm, fsm2, NULL);
		assert(fsm != NULL);
	}

	fsm_increndids(fsm, 10); // now endids are 11 and 12

	ret = fsm_determinise(fsm);
	assert(ret != 0);

	ret = fsm_minimise(fsm);
	assert(ret != 0);

	/* find end states, make sure we have two end states and they each have endids */
	nstates = fsm_countstates(fsm);
	nend    = fsm_count(fsm, fsm_isend);

	info = calloc(nstates, sizeof *info);
	assert(info != NULL);
	ninfo = 0;

	for (state_ind = 0; state_ind < nstates; state_ind++) {
		if (fsm_isend(fsm, state_ind)) {
			fsm_end_id_t endids[2] = {0,0};
			size_t nwritten;
			size_t num_endids;
			enum fsm_getendids_res ret;

			nwritten = 0;
			num_endids = fsm_getendidcount(fsm, state_ind);

			assert( num_endids > 0 && num_endids <= 2);

			ret = fsm_getendids(
				fsm,
				state_ind,
				sizeof endids / sizeof endids[0],
				&endids[0],
				&nwritten);

			assert(ret == FSM_GETENDIDS_FOUND);
			assert(nwritten == num_endids);

			info[ninfo].state = state_ind;
			info[ninfo].num_endids = nwritten;

			if (nwritten == 1) {
				assert(endids[0] == 11 || endids[0] == 12);
				info[ninfo].endids[0] = endids[0];
			} else if (nwritten == 2) {
				qsort(&endids[0], nwritten, sizeof endids[0], cmp_endids);
				assert(endids[0] == 11 && endids[1] == 12);
				info[ninfo].endids[0] = endids[0];
				info[ninfo].endids[1] = endids[1];
			}

			ninfo++;
		}
	}

	assert( ninfo == nend );

        fsm_free(fsm);
        free(info);

        printf("ok\n");

	return EXIT_SUCCESS;
}

