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

/* Test that providing an overly large endid buffer doesn't clobber the result with stale data. */
int main(void)
{
	const char *input = "banana turtle";

	const char *s = input;
	struct fsm *fsm = re_comp(RE_NATIVE, fsm_sgetc, &s, NULL, 0, NULL);
	int ret;

	ret = fsm_setendid(fsm, (fsm_end_id_t) 12345);
	assert(ret == 1);

        ret = fsm_determinise(fsm);
        assert(ret == 1);

        ret = fsm_minimise(fsm);
        assert(ret == 1);

	fsm_state_t end;
	ret = fsm_exec(fsm, fsm_sgetc, &input, &end, NULL);
	assert(ret == 1);

	const size_t endid_count = fsm_endid_count(fsm, end);
	assert(endid_count == 1);

	/* Regression: fsm_endid_get previously qsort'd endids with
	 * a count based on the buffer size (here ENDID_BUF_CEIL),
	 * rather than the number of results populated, so if the
	 * buffer was larger than fsm_endid_count's result stale
	 * contents could displace the correct results. */
#define ENDID_BUF_CEIL 10
	fsm_end_id_t endids[ENDID_BUF_CEIL] = { 333 };
	ret = fsm_endid_get(fsm, end, ENDID_BUF_CEIL, endids);
	assert(endids[0] == 12345);

	fsm_free(fsm);
	return EXIT_SUCCESS;
}
