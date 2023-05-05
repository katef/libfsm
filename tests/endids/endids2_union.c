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

#include "endids_utils.h"

/* test that endids correctly propagate through union */
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

	ret = fsm_setendid(fsm1, (fsm_end_id_t) 1);
	assert(ret == 1);

	ret = fsm_setendid(fsm2, (fsm_end_id_t) 2);
	assert(ret == 1);

	comb = fsm_union(fsm1, fsm2, NULL);
	assert(comb != NULL);

	ret = fsm_determinise(comb);
	assert(ret != 0);

	// find end states, make sure we have two end states and they each have endids
	nstates = fsm_countstates(comb);

	// make sure we have both end ids
	int has_endid_1 = 0;
	int has_endid_2 = 0;

	memset(all_endids, 0, sizeof all_endids);
	for (state_ind = 0; state_ind < nstates; state_ind++) {
		if (fsm_isend(comb, state_ind)) {
			fsm_end_id_t endids[2] = {0,0};
			size_t nwritten;
			size_t num_endids;
			enum fsm_getendids_res ret;

			nwritten = 0;
			num_endids = fsm_getendidcount(comb, state_ind);
			// fprintf(stderr, "state %u, num_endids = %zu\n", state_ind, num_endids);

			assert( num_endids > 0 && num_endids <= 2);

			ret = fsm_getendids(
				comb,
				state_ind,
				2,
				&endids[0],
				&nwritten);

			assert(ret == FSM_GETENDIDS_FOUND);
			assert(nwritten == num_endids);

			if (nwritten == 1) {
				assert(endids[0] == 1 || endids[0] == 2);

				has_endid_1 = has_endid_1 || (endids[0] == 1);
				has_endid_2 = has_endid_2 || (endids[0] == 2);
			} else if (nwritten == 2) {
				assert((endids[0] == 1 && endids[1] == 2)
					|| (endids[0] == 2 && endids[1] == 1));

				has_endid_1 = has_endid_1 || (endids[0] == 1) || (endids[1] == 1);
				has_endid_2 = has_endid_2 || (endids[0] == 2) || (endids[1] == 2);
			}

			/*
			// all_endids[nend++] = endid;
			fprintf(stderr, "state %u has %zu end ids:", state_ind, nendids);
			for (i=0; i < nendids; i++) {
				fprintf(stderr, " %u", (unsigned) endids[i]);
			}
			fprintf(stderr, "\n");
			*/
		}
	}

	assert( has_endid_1 );
	assert( has_endid_2 );

	/* execute the fsm and make sure that the right end states correspond to the
	 * right endids
	 */

        {
		size_t i;
		static const struct {
			const char *s;
			int should_match;
			size_t num_endids;
			fsm_end_id_t endid[2];
		} matches[] = {
			{ "abc"      , 1, 1, { 1, 0 } },
			{ " abc"     , 1, 1, { 1, 0 } },
			{ " abc "    , 1, 1, { 1, 0 } },
			{ "abc "     , 1, 1, { 1, 0 } },
			{ "abb"      , 0, 0, { 0, 0 } },
			{ "abbc"     , 0, 0, { 0, 0 } },

			{ "def"      , 1, 1, { 2, 0 } },
			{ " def"     , 1, 1, { 2, 0 } },
			{ "def "     , 1, 1, { 2, 0 } },
			{ " def "    , 1, 1, { 2, 0 } },

			{ "ddf"      , 0, 0, { 0, 0 } },

			{ "abcdef"   , 1, 2, { 1, 2 } },
			{ "defabc"   , 1, 2, { 1, 2 } },
			{ " abcdef"  , 1, 2, { 1, 2 } },
			{ "abcdef "  , 1, 2, { 1, 2 } },
			{ "abc def"  , 1, 2, { 1, 2 } },
			{ " abc def ", 1, 2, { 1, 2 } },
			{ " defabc"  , 1, 2, { 1, 2 } },
			{ "defabc "  , 1, 2, { 1, 2 } },
			{ " defabc " , 1, 2, { 1, 2 } },
			{ "def abc"  , 1, 2, { 1, 2 } },
			{ " def abc ", 1, 2, { 1, 2 } },

			{ "ab cdef"  , 1, 1, { 2, 0 } },
			{ "abcd ef"  , 1, 1, { 1, 0 } },
			{ "ab cd ef" , 0, 0, { 0, 0 } },
			{ "adbecf"   , 0, 0, { 0, 0 } },
		};

		for (i=0; i < sizeof matches / sizeof matches[0]; i++) {
			fsm_end_id_t *end_ids;
			size_t num_end_ids;

			end_ids = NULL;
			num_end_ids = 0;
			ret = match_string(comb, matches[i].s, NULL, &end_ids, &num_end_ids);

			if (matches[i].should_match) {
				size_t j;

				assert( ret == 1 );
				assert( end_ids != NULL );

				assert( num_end_ids == matches[i].num_endids );

				qsort(&end_ids[0], num_end_ids, sizeof end_ids[0], cmp_endids);

				for (j=0; j < num_end_ids; j++) {
					assert( end_ids[j] == matches[i].endid[j] );
				}
			} else {
				assert( ret == 0 );
				assert( end_ids == NULL );
				assert( num_end_ids == 0 );
			}

			free(end_ids);
		}
        }

        fsm_free(comb);

        printf("ok\n");

	return EXIT_SUCCESS;
}

