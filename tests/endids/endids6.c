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

/* remap 1 -> 512, 2 -> 1024 */
static int
endid_remap_func(fsm_state_t state, size_t num_ids, fsm_end_id_t *endids, size_t *num_written, void *opaque)
{
	size_t i;

	(void)state;
	(void)opaque;

	assert(endids != NULL);
	for (i=0; i < num_ids; i++) {
		fsm_end_id_t orig = endids[i];

		switch (endids[i]) {
		case 1: endids[i] =  512; break;
		case 2: endids[i] = 1024; break;
		default: break;
		}

		printf("remap: state %u id %u -> %u\n",
			(unsigned)state, (unsigned)orig, (unsigned)endids[i]);
	}

	*num_written = num_ids;

	return 1;
}

/* test that we can remap endids */
int main(void)
{
	struct fsm *fsm;
	const char *s;
	struct state_info *info;
	size_t nstates, ninfo, state_ind, info_ind, nend;
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
	}

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
				assert(endids[0] == 1 || endids[0] == 2);
				info[ninfo].endids[0] = endids[0];
			} else if (nwritten == 2) {
				qsort(&endids[0], nwritten, sizeof endids[0], cmp_endids);
				assert(endids[0] == 1 && endids[1] == 2);
				info[ninfo].endids[0] = endids[0];
				info[ninfo].endids[1] = endids[1];
			}

			ninfo++;
		}
	}

	assert( ninfo == nend );

	/* remap 1 -> 512, 2 -> 1024 */
	ret = fsm_mapendids(fsm, endid_remap_func, NULL);
	assert( ret == 1 );

	for (state_ind = 0, info_ind = 0; state_ind < nstates; state_ind++) {
		if (fsm_isend(fsm, state_ind)) {
			fsm_end_id_t endids[2] = {0,0};
			size_t nwritten, num_endids, ind;
			enum fsm_getendids_res ret;

			assert( state_ind == info[info_ind].state );

			nwritten = 0;
			num_endids = fsm_getendidcount(fsm, state_ind);

			assert(num_endids > 0 && num_endids <= 2);

			assert( num_endids == info[info_ind].num_endids );
			ret = fsm_getendids(
				fsm,
				state_ind,
				sizeof endids / sizeof endids[0],
				&endids[0],
				&nwritten);

			assert(ret == FSM_GETENDIDS_FOUND);
			assert(nwritten == num_endids);

			if (nwritten > 1) {
				qsort(&endids[0], num_endids, sizeof endids[0], cmp_endids);
			}

			for (ind=0; ind < num_endids; ind++) {
				fsm_end_id_t expected = info[info_ind].endids[ind];
				switch (info[info_ind].endids[ind]) {
				case 1: expected = 512;  break;
				case 2: expected = 1024; break;
				default:
					assert( !"unexpected/invalid value" );
					break;
				}

				printf("state %u, id %u, expected %u\n", (unsigned)state_ind, endids[ind], expected);
				assert(endids[ind] == expected);
			}

			info_ind++;
		}
	}

        fsm_free(fsm);

        printf("ok\n");

	return EXIT_SUCCESS;
}

