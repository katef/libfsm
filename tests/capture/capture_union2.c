/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/capture.h>
#include <fsm/print.h>

#include "captest.h"

/* union /(abcd)/ and /(abed)/ */

static struct fsm *
build(unsigned *cb_a, unsigned *cb_b);

static void
check(const struct fsm *fsm, const char *input,
    unsigned end_id, unsigned exp_capture_id,
    size_t exp_start, size_t exp_end);

int main(void) {
	unsigned cb_abcd, cb_abed;
	struct fsm *fsm = build(&cb_abcd, &cb_abed);

	check(fsm, "abcd", 0, cb_abcd, 0, 4);
	check(fsm, "abed", 1, cb_abed, 0, 4);

	fsm_free(fsm);

	return EXIT_SUCCESS;
}

static struct fsm *
build(unsigned *cb_a, unsigned *cb_b)
{
	struct fsm *abcd = captest_fsm_of_string("abcd", 0);
	struct fsm *abed = captest_fsm_of_string("abed", 1);
	struct fsm *res;

	assert(abcd);
	assert(abed);

	if (!fsm_capture_set_path(abcd, 0, 0, 4)) {
		assert(!"path 0");
	}
	if (!fsm_capture_set_path(abed, 0, 0, 4)) {
		assert(!"path 1");
	}

	{
		struct fsm *fsms[2];
		struct fsm_combined_base_pair bases[2];
		fsms[0] = abcd;
		fsms[1] = abed;
		res = fsm_union_array(2, fsms, bases);
		assert(res);
		*cb_a = bases[0].capture;
		*cb_b = bases[1].capture;
	}

	if (!fsm_determinise(res)) {
		assert(!"determinise");
	}

	assert(fsm_countcaptures(res) == 2);

	return res;
}

static void
check(const struct fsm *fsm, const char *input,
    unsigned end_id, unsigned exp_capture_id,
    size_t exp_start, size_t exp_end)
{
	struct captest_input ci;
	fsm_state_t end;
	int exec_res;
	struct fsm_capture got_captures[MAX_TEST_CAPTURES];

	ci.string = input;
	ci.pos = 0;

	exec_res = fsm_exec(fsm, captest_getc, &ci, &end, got_captures);
	if (exec_res != 1) {
		fprintf(stderr, "exec_res: %d\n", exec_res);
		exit(EXIT_FAILURE);
	}

	{
		const char *msg;
		if (!captest_check_single_end_id(fsm, end, end_id, &msg)) {
			fprintf(stderr, "%s\n", msg);
			exit(EXIT_FAILURE);
		}
	}

	if (got_captures[exp_capture_id].pos[0] != exp_start) {
		fprintf(stderr, "capture[%u].pos[0]: exp %lu, got %lu\n",
		    exp_capture_id, exp_start,
		    got_captures[exp_capture_id].pos[0]);
		exit(EXIT_FAILURE);
	}
	if (got_captures[exp_capture_id].pos[1] != exp_end) {
		fprintf(stderr, "capture[%u].pos[1]: exp %lu, got %lu\n",
		    exp_capture_id, exp_end,
		    got_captures[exp_capture_id].pos[1]);
		exit(EXIT_FAILURE);
	}
}
