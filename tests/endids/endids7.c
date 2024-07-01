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
	unsigned count;
	fsm_end_id_t ids[2];
};

/* remap 1 -> 7, 2 -> 9, both 1&2 -> 3 */
static int
endid_remap_func(fsm_state_t state, size_t num_ids, fsm_end_id_t *ids, size_t *num_written, void *opaque)
{
	(void)state;
	(void)opaque;

	assert(ids != NULL);
	assert(num_ids > 0 && num_ids <= 2);

	if (num_ids > 1) {
		ids[0] = 3;
		*num_written = 1;

		printf("remap: state %u with %zu endids, set to one endid, value %u\n",
			(unsigned)state, num_ids, (unsigned)ids[0]);

		return 1;
	}

	if (num_ids == 1) {
		fsm_end_id_t orig = ids[0];

		switch (ids[0]) {
		case 1: ids[0] = 7; break;
		case 2: ids[0] = 9; break;
		default: break;
		}

		printf("remap: state %u id %u -> %u\n",
			(unsigned)state, (unsigned)orig, (unsigned)ids[0]);
	}

	*num_written = num_ids;

	return 1;
}

/* test that remapping endids can decrease the number of
 * endids
 */
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
			fsm_end_id_t ids[2] = {0,0};
			size_t count;
			int ret;

			count = fsm_endid_count(fsm, state_ind);
			assert( count > 0 && count <= 2);

			ret = fsm_endid_get(fsm, state_ind, count, &ids[0]);
			assert(ret == 1);

			info[ninfo].state = state_ind;
			info[ninfo].count = count;

			if (count == 1) {
				assert(ids[0] == 1 || ids[0] == 2);
				info[ninfo].ids[0] = ids[0];
			} else if (count == 2) {
				assert(ids[0] == 1 && ids[1] == 2);
				info[ninfo].ids[0] = ids[0];
				info[ninfo].ids[1] = ids[1];
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
			fsm_end_id_t id = 0;
			fsm_end_id_t expected;
			size_t count;
			int ret;

			assert( state_ind == info[info_ind].state );

			count = fsm_endid_count(fsm, state_ind);
			assert(count == 1);

			ret = fsm_endid_get(fsm, state_ind, 1, &id);
			assert(ret == 1);

			if (info[info_ind].count == 2) {
				expected = 3;
			} else if (info[info_ind].ids[0] == 1) {
				expected = 7;
			} else if (info[info_ind].ids[0] == 2) {
				expected = 9;
			} else {
				assert( ! "unexpected value" );
			}

			printf("state %u, id %u, expected %u\n", (unsigned)state_ind, id, expected);
			assert(id == expected);;

			info_ind++;
		}
	}

        fsm_free(fsm);
        free(info);

        printf("ok\n");

	return EXIT_SUCCESS;
}

