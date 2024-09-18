#include "testutil.h"

#include <string.h>

#include <re/re.h>
#include <fsm/fsm.h>
#include <fsm/options.h>
#include <fsm/walk.h>
#include <fsm/print.h>
#include <fsm/pred.h>

#include <adt/u64bitset.h>

bool
run_test(const struct testcase *tc)
{
	bool test_res = false;

	enum re_flags flags = 0;
	struct re_err err;
	char *regex = (char *)tc->regex;
	const char *required = tc->required ? tc->required : "";
	const size_t step_limit = tc->step_limit ? tc->step_limit : DEF_STEP_LIMIT;

	fprintf(stderr, "-- test: regex '%s', required '%s'\n", tc->regex, required);

	struct fsm *fsm = re_comp(RE_PCRE, fsm_sgetc, &regex, NULL, flags, &err);
	if (fsm == NULL) {
		return false;
	}

	if (!fsm_determinise(fsm)) {
		assert(!"determinise");
		return false;
	}
	if (!fsm_minimise(fsm)) {
		assert(!"minimise");
		return false;
	}

	uint64_t charmap[4];

	{
		const size_t statecount = fsm_countstates(fsm);
		size_t ends = 0;
		for (size_t i = 0; i < statecount; i++) {
			if (fsm_isend(fsm, i)) {
				ends++;
			}
		}
		fprintf(stderr, "-- statecount %zu, %zu ends\n", statecount, ends);
	}


	const enum fsm_detect_required_characters_res res = fsm_detect_required_characters(fsm, step_limit, charmap, NULL);
	if (res == FSM_DETECT_REQUIRED_CHARACTERS_STEP_LIMIT_REACHED) {
		fprintf(stderr, "-- step limit reached, halting\n");
		goto cleanup;
	}
	assert(res == FSM_DETECT_REQUIRED_CHARACTERS_WRITTEN);

	char buf[257] = {0};
	size_t used = 0;
	assert(!u64bitset_get(charmap, 0)); /* does not contain 0x00 */

	for (size_t i = 0; i < 256; i++) {
		if (u64bitset_get(charmap, i)) {
			buf[used++] = (char)i;
		}
	}

	if (0 != strcmp(required, buf)) {
		fprintf(stderr, "Error: mismatch\n");
		fprintf(stderr, "-- expected: [%s]\n", required);
		fprintf(stderr, "-- got: [%s]\n", buf);
		goto cleanup;
	}

	test_res = true;

cleanup:
	fsm_free(fsm);

	return test_res;
}
