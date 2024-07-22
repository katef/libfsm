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
#include <fsm/capture.h>
#include <fsm/bool.h>

#include "captest.h"

/* concat /(abc)/ and /(de)/ */

static struct fsm *
build(unsigned *cb_a, unsigned *cb_b);

static void
check(const struct fsm *fsm, const char *input, unsigned end_id,
    unsigned cb_ab, size_t exp_start_ab, size_t exp_end_ab,
    unsigned cb_cde, size_t exp_start_cde, size_t exp_end_cde);

int main(void) {
	unsigned cb_abc, cb_de; /* capture base */
	struct fsm *abcde = build(&cb_abc, &cb_de);

	check(abcde, "abcde", 1,
	    cb_abc, 0, 3,
	    cb_de, 3, 5);

	fsm_free(abcde);

	return EXIT_SUCCESS;
}

static struct fsm *
build(unsigned *cb_a, unsigned *cb_b)
{
	struct fsm *abc = captest_fsm_of_string("abc", 0);
	struct fsm *de = captest_fsm_of_string("de", 1);
	struct fsm *abcde;
	struct fsm_combine_info ci;
	size_t cc_abc, cc_de, cc_abcde;

	assert(abc);
	assert(de);

	if (!fsm_capture_set_path(abc, 0, 0, 3)) {
		assert(!"path 0");
	}
	if (!fsm_capture_set_path(de, 0, 0, 2)) {
		assert(!"path 1");
	}

	cc_abc = fsm_countcaptures(abc);
	assert(cc_abc == 1);

	cc_de = fsm_countcaptures(de);
	assert(cc_de == 1);

	abcde = fsm_concat(abc, de, &ci);
	assert(abcde);
	*cb_a = ci.capture_base_a;
	*cb_b = ci.capture_base_b;

	cc_abcde = fsm_countcaptures(abcde);
	assert(cc_abcde == cc_abc + cc_de);

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "==== after concat: cb_abc %u, cb_de %u\n",
	    *cb_a, *cb_b);
	fsm_dump(stderr, abcde);

	fsm_capture_dump(stderr, "#### after concat", abcde);

	fprintf(stderr, "==== determinise\n");
#endif

	if (!fsm_determinise(abcde)) {
		assert(!"determinise");
	}

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "==== after determinise\n");
	fsm_dump(stderr, abcde);

	assert(fsm_countcaptures(abcde) == cc_abcde);

	fsm_capture_dump(stderr, "#### after det", abcde);
#endif

	assert(fsm_countcaptures(abcde) == cc_abcde);
	return abcde;
}

static void
check(const struct fsm *fsm, const char *input, unsigned end_id,
    unsigned cb_abc, size_t exp_start_abc, size_t exp_end_abc,
    unsigned cb_de, size_t exp_start_de, size_t exp_end_de)
{
	struct captest_input ci;
	fsm_state_t end;
	int exec_res;
	struct fsm_capture captures[MAX_TEST_CAPTURES];

	ci.string = input;
	ci.pos = 0;

	exec_res = fsm_exec(fsm, captest_getc, &ci, &end, captures);
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

	assert(captures[cb_abc].pos[0] == exp_start_abc);
	assert(captures[cb_abc].pos[1] == exp_end_abc);

	assert(captures[cb_de].pos[0] == exp_start_de);
	assert(captures[cb_de].pos[1] == exp_end_de);
}
