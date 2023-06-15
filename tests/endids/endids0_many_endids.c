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

/* basic test that we can set and retrieve end ids */
int main(void)
{
	struct fsm *fsm;
	const char *s;
	int ret;
	size_t nend;
	size_t nstates, state_ind;
	size_t i;
	fsm_state_t end_state;

	s = "abc"; fsm = re_comp(RE_NATIVE, fsm_sgetc, &s, NULL, 0, NULL);

	ret = fsm_determinise(fsm);
	assert(ret == 1);

        const unsigned endcount = fsm_count(fsm, fsm_isend);

	/* check that we can correctly set lots of end ids, in no particular order
	 *
	 * 17 endids.  This should force the array of end ids to resize.
	 */
	static const fsm_end_id_t all_end_ids[17] = {
		4, 17,  6, 18,  2, 63, 14, 62,
		3, 37, 46,  7,  9, 72, 67, 36,
		1,
	};

	static const fsm_end_id_t sorted_all_end_ids[17] = {
		1, 2, 3, 4, 6, 7, 9, 14, 17, 18, 36, 37, 46, 62, 63, 67, 72,
	};

	for (size_t i=0; i < sizeof all_end_ids / sizeof all_end_ids[0]; i++) {
		ret = fsm_setendid(fsm, all_end_ids[i]);
		assert(ret == 1);
	}

	/* find end states, make sure we have two end states and they each have endids */
	nstates = fsm_countstates(fsm);

	/* find an end state */
	end_state = nstates; /* invalid state index */

	for (state_ind = 0; state_ind < nstates; state_ind++) {
		if (fsm_isend(fsm, state_ind)) {
			end_state = state_ind;
			break;
		}
	}

	assert(end_state < nstates);

	assert(fsm_getendidcount(fsm, end_state) == sizeof all_end_ids / sizeof all_end_ids[0]);
	for (i=0; i < sizeof all_end_ids / sizeof all_end_ids[0]; i++) {
		/* add duplicate end ids */
		ret = fsm_setendid(fsm, all_end_ids[i]);

		/* fsm_setendid should succeed */
		assert(ret == 1);

		/* but it shouldn't add a duplicate id */
		assert(fsm_getendidcount(fsm, end_state) == sizeof all_end_ids / sizeof all_end_ids[0]);
	}

	nend = 0;
	for (state_ind = 0; state_ind < nstates; state_ind++) {
		if (fsm_isend(fsm, state_ind)) {
			fsm_end_id_t endids[sizeof all_end_ids / sizeof all_end_ids[0]];
			size_t i, nwritten;
			enum fsm_getendids_res ret;

			memset(&endids[0], 0, sizeof endids);

			assert(fsm_getendidcount(fsm, state_ind) == sizeof all_end_ids / sizeof all_end_ids[0]);

                        nwritten = 0;
			ret = fsm_getendids(
				fsm,
				state_ind,
				sizeof endids/sizeof endids[0],
				&endids[0],
				&nwritten);

			assert(ret == FSM_GETENDIDS_FOUND);
                        assert(nwritten == sizeof all_end_ids / sizeof all_end_ids[0]);

                        /* sort endids and compare */
                        qsort(&endids[0], nwritten, sizeof endids[0], cmp_endids);
			for (i=0; i < nwritten; i++) {
				assert(endids[i] == sorted_all_end_ids[i]);
			}

#if 0
			/* endids should be sorted */
			for (i=0; i < nwritten; i++) {
				assert(endids[i] == sorted_all_end_ids[i]);
			}
#endif /* 0 */

			nend++;
		}
	}

	assert( nend == endcount );

	/* execute the fsm and make sure that the right end states correspond to the
	 * right endids
	 */

        {
            size_t i;
            static const struct {
                const char *s;
                int should_match;
                fsm_end_id_t endid;
            } matches[] = {
                { "abc", 1, 1 },
                { " abc", 1, 1 },
                { " abc ", 1, 1 },
                { "abc ", 1, 1 },
                { "abb", 0, 0 },
                { "abbc", 0, 0 },
            };

            for (i=0; i < sizeof matches / sizeof matches[0]; i++) {
		fsm_end_id_t *end_ids;
		size_t num_end_ids;

		end_ids = NULL;
		num_end_ids = 0;
		ret = match_string(fsm, matches[i].s, NULL, &end_ids, &num_end_ids);

                if (matches[i].should_match) {
			size_t j;

			assert( ret == 1 );
			assert( end_ids != NULL );
			assert( num_end_ids == sizeof all_end_ids / sizeof all_end_ids[0] );

#if 0
			assert( end_ids[0] == 1 );
			assert( end_ids[0] == matches[i].endid );
#endif /* 0 */

                        /* sort endids and compare */
                        qsort(&end_ids[0], num_end_ids, sizeof end_ids[0], cmp_endids);

			for (j=0; j < num_end_ids; j++) {
				assert(end_ids[j] == sorted_all_end_ids[j]);
			}
                } else {
			assert( ret == 0 );
			assert( end_ids == NULL );
			assert( num_end_ids == 0 );
                }

		free(end_ids);
            }
        }

        fsm_free(fsm);

        printf("ok\n");

	return EXIT_SUCCESS;
}

