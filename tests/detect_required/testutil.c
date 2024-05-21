#include "testutil.h"

#include <string.h>

#include <re/re.h>
#include <fsm/fsm.h>
#include <fsm/options.h>
#include <fsm/walk.h>
#include <adt/bitmap.h>
#include <fsm/print.h>

#include <fsm/pred.h>

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

	struct bm bitmap;
	bm_clear(&bitmap);

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


	const enum fsm_detect_required_characters_res res = fsm_detect_required_characters(fsm, step_limit, &bitmap);
	if (res == FSM_DETECT_REQUIRED_CHARACTERS_STEP_LIMIT_REACHED) {
		fprintf(stderr, "-- step limit reached, halting\n");
		goto cleanup;
	}
	assert(res == FSM_DETECT_REQUIRED_CHARACTERS_WRITTEN);

	char buf[257] = {0};
	size_t used = 0;
	assert(!bm_get(&bitmap, 0)); /* does not contain 0x00 */

	int i = 0;
	for (;;) {
		const size_t next = bm_next(&bitmap, i, 1);
		if (next > UCHAR_MAX) { break; }
		buf[used++] = (char)next;
		i = next;
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
