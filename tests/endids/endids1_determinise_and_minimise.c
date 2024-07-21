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

/* test that endids are correctly propagated through fsm_determinise() 
 * and fsm_minimise()
 */
int main(void)
{
	struct fsm *fsm;
	const char *s;
	int ret;
	size_t nstates, state_ind;

	s = "abc"; fsm = re_comp(RE_NATIVE, fsm_sgetc, &s, NULL, 0, NULL);

	ret = fsm_setendid(fsm, (fsm_end_id_t) 1);
	assert(ret == 1);

	ret = fsm_determinise(fsm);
	assert(ret == 1);

	ret = fsm_minimise(fsm);
	assert(ret == 1);

	// find end states, make sure we have two end states and they each have endids
	nstates = fsm_countstates(fsm);

	// memset(all_endids, 0, sizeof all_endids);
	for (state_ind = 0; state_ind < nstates; state_ind++) {
		if (fsm_isend(fsm, state_ind)) {
			fsm_end_id_t endid = 0;
			int ret;

			assert(fsm_endid_count(fsm, state_ind) == 1);

			ret = fsm_endid_get(fsm, state_ind, 1, &endid);
			assert(ret == 1);
			assert( endid == 1 );
		}
	}

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
		fsm_end_id_t *ids;
		size_t count;

		ids = NULL;
		count = 0;
		ret = match_string(fsm, matches[i].s, NULL, &ids, &count);

                if (matches[i].should_match) {
                    assert( ret == 1 );
                    assert( ids != NULL );
                    assert( ids[0] = 1 );
                    assert( count == 1 );
                    assert( ids[0] = matches[i].endid );
                } else {
                    assert( ret == 0 );
                    assert( ids == NULL );
                    assert( count == 0 );
                }

		free(ids);
            }
        }

        fsm_free(fsm);

        printf("ok\n");

	return EXIT_SUCCESS;
}


