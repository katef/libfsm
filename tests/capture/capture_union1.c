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

/* union /(ab)/ and /(cde)/ */

static struct fsm *
build(unsigned *cb_a, unsigned *cb_b);

static void
check(const struct fsm *fsm, const char *input,
    unsigned end_id, unsigned exp_capture_id,
    size_t exp_start, size_t exp_end);

int main(void) {
	unsigned cb_ab, cb_cde; /* capture base */
	struct fsm *abcde = build(&cb_ab, &cb_cde);

	check(abcde, "ab", 0, cb_ab, 0, 2);
	check(abcde, "cde", 1, cb_cde, 0, 3);

	fsm_free(abcde);

	return EXIT_SUCCESS;
}

static struct fsm *
build(unsigned *cb_a, unsigned *cb_b)
{
	struct fsm *ab = captest_fsm_of_string("ab", 0);
	struct fsm *cde = captest_fsm_of_string("cde", 1);
	struct fsm *abcde;
	struct fsm_combine_info ci;
	size_t cc_ab, cc_cde, cc_abcde;

	assert(ab);
	assert(cde);

	if (!fsm_capture_set_path(ab, 0, 0, 2)) {
		assert(!"path 0");
	}
	if (!fsm_capture_set_path(cde, 0, 0, 3)) {
		assert(!"path 1");
	}

	cc_ab = fsm_countcaptures(ab);
	assert(cc_ab == 1);

	cc_cde = fsm_countcaptures(cde);
	assert(cc_cde == 1);

	abcde = fsm_union(ab, cde, &ci);
	assert(abcde);
	*cb_a = ci.capture_base_a;
	*cb_b = ci.capture_base_b;

	cc_abcde = fsm_countcaptures(abcde);
	assert(cc_abcde == cc_ab + cc_cde);

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "==== after union: cb_ab %u, cb_cde %u\n",
	    *cb_a, *cb_b);
	fsm_print_fsm(stderr, abcde);

	fsm_capture_dump(stderr, "#### after union", abcde);

	fprintf(stderr, "==== determinise\n");
#endif

	if (!fsm_determinise(abcde)) {
		assert(!"determinise");
	}

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "==== after determinise\n");
	fsm_print_fsm(stderr, abcde);

	assert(fsm_countcaptures(abcde) == cc_abcde);

	fsm_capture_dump(stderr, "#### after det", abcde);
#endif

	assert(fsm_countcaptures(abcde) == cc_abcde);
	return abcde;
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
