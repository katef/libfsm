#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <errno.h>

#include <assert.h>

#include <re/re.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/walk.h>

#include "endids_utils.h"

#define ENDID_AB_STAR_C 1
#define ENDID_ABC 2

int main(void)
{
	/* Union /^ab*c$/ and /^abc$/ with distinct endids set on each
	 * and verify that inputs of "ac", "abc", "abbc" get the endids
	 * associated with only the regexes that match.
	 *
	 * In other words, check that minimisation does not merge them
	 * and cause false positives. */

	const char *regex_ab_star_c = "^ab*c$";
	const char *regex_abc = "^abc$";

	struct fsm *fsm_ab_star_c = re_comp(RE_NATIVE, fsm_sgetc, (void *)&regex_ab_star_c, NULL, 0, NULL);
	assert(fsm_ab_star_c != NULL);
	if (!fsm_setendid(fsm_ab_star_c, ENDID_AB_STAR_C)) { assert(!"setendid"); }

	if (!fsm_determinise(fsm_ab_star_c)) { assert(!"determinise"); }
	if (!fsm_minimise(fsm_ab_star_c)) { assert(!"minimise"); }

	struct fsm *fsm_abc = re_comp(RE_NATIVE, fsm_sgetc, (void *)&regex_abc, NULL, 0, NULL);
	assert(fsm_abc != NULL);
	if (!fsm_setendid(fsm_abc, ENDID_ABC)) { assert(!"setendid"); }

	if (!fsm_determinise(fsm_abc)) { assert(!"determinise"); }
	if (!fsm_minimise(fsm_abc)) { assert(!"minimise"); }

	struct fsm *combined = fsm_union(fsm_ab_star_c, fsm_abc, NULL);
	assert(combined != NULL);

	int ret = fsm_determinise(combined);
	assert(ret != 0);

	ret = fsm_minimise(combined);
	assert(ret != 0);

	size_t written;
	fsm_end_id_t *endids = NULL;
	if (match_string(combined, "ac", NULL, &endids, &written) != 1) {
		assert(!"'ac' should match");
	}
	assert(written == 1);
	assert(endids[0] == ENDID_AB_STAR_C);
	free(endids);

	if (match_string(combined, "abc", NULL, &endids, &written) != 1) {
		assert(!"'abc' should match");
	}
	assert(written == 2);
	/* result is not sorted */
	assert((endids[0] == ENDID_AB_STAR_C && endids[1] == ENDID_ABC) ||
	    (endids[1] == ENDID_AB_STAR_C && endids[0] == ENDID_ABC));
	free(endids);

	if (match_string(combined, "abbc", NULL, &endids, &written) != 1) {
		assert(!"'abbc' should match");
	}
	assert(written == 1);
	assert(endids[0] == ENDID_AB_STAR_C);
	free(endids);

	fsm_free(combined);
}
