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
#include <fsm/pred.h>
#include <fsm/print.h>

#include "captest.h"

/* Combine 2 mostly overlapping FSMs:
 * - 0: "(abc)"
 * - 1: "(ab*c)"
 * and check for false positives in the match.
 */

static struct fsm *
build_and_combine(unsigned *cb_a, unsigned *cb_b);

static void
det_and_min(const char *tag, struct fsm *fsm);

static struct fsm *
build_ab_c(void);

static void
check(const struct fsm *fsm, const char *string,
    unsigned expected_ends,
    unsigned cb_a, size_t pa_0, size_t pa_1,
    unsigned cb_b, size_t pb_0, size_t pb_1);

int main(void) {
	unsigned cb_abc, cb_ab_c;
	struct fsm *f_all = build_and_combine(&cb_abc, &cb_ab_c);
	unsigned captures;
	const unsigned exp_0 = 1U << 0;
	const unsigned exp_1 = 1U << 1;

	captures = fsm_countcaptures(f_all);
	if (captures != 2) {
		fprintf(stderr, "expected 2 captures, got %u\n", captures);
		exit(EXIT_FAILURE);
	}

	#define NO_POS FSM_CAPTURE_NO_POS
	check(f_all, "abc",	/* captures 0 and 1 */
	    exp_0 | exp_1,
	    cb_abc, 0, 3,
	    cb_ab_c, 0, 3);
	check(f_all, "ac",	/* only capture 1 */
	    exp_1,
	    cb_abc, NO_POS, NO_POS,
	    cb_ab_c, 0, 2);
	check(f_all, "abbc",	/* only capture 1 */
	    exp_1,
	    cb_abc, NO_POS, NO_POS,
	    cb_ab_c, 0, 4);

	fsm_free(f_all);

	return 0;
}

static struct fsm *
build_and_combine(unsigned *cb_a, unsigned *cb_b)
{
	struct fsm *f_abc = captest_fsm_of_string("abc", 0);
	struct fsm *f_ab_c = build_ab_c();
	struct fsm *f_all;
	struct fsm_combine_info ci;

	assert(f_abc);
	assert(f_ab_c);

	if (!fsm_capture_set_path(f_abc, 0, 0, 3)) {
		exit(EXIT_FAILURE);
	}
	if (!fsm_capture_set_path(f_ab_c, 0, 0, 3)) {
		exit(EXIT_FAILURE);
	}

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "==================== abc \n");
	fsm_dump(stderr, f_abc);
	fsm_capture_dump(stderr, "abc", f_abc);

	fprintf(stderr, "==================== ab*c \n");
	fsm_dump(stderr, f_ab_c);
	fsm_capture_dump(stderr, "ab*c", f_ab_c);
#endif

	det_and_min("abc", f_abc);
	det_and_min("ab*c", f_ab_c);

	/* union them */
	f_all = fsm_union(f_abc, f_ab_c, &ci);
	assert(f_all != NULL);

	*cb_a = ci.capture_base_a;
	*cb_b = ci.capture_base_b;

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "==================== post-union \n");
	fsm_dump(stderr, f_all);
	fsm_capture_dump(stderr, "capture_actions", f_all);
	fprintf(stderr, "====================\n");
#endif

	if (!fsm_determinise(f_all)) {
		exit(EXIT_FAILURE);
	}

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "==================== post-det \n");
	fsm_dump(stderr, f_all);
	fsm_capture_dump(stderr, "capture_actions", f_all);
	fprintf(stderr, "====================\n");
#endif

	return f_all;
}

static void
det_and_min(const char *tag, struct fsm *fsm)
{
	if (!fsm_determinise(fsm)) {
		fprintf(stderr, "Failed to determise '%s'\n", tag);
		exit(EXIT_FAILURE);
	}

	if (!fsm_minimise(fsm)) {
		fprintf(stderr, "Failed to minimise '%s'\n", tag);
		exit(EXIT_FAILURE);
	}

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "==== after det_and_min: '%s'\n", tag);
	fsm_dump(stderr, fsm);
	fsm_capture_dump(stderr, tag, fsm);
#endif

}

static struct fsm *
build_ab_c(void)
{
	struct fsm *fsm = fsm_new(NULL);
	assert(fsm != NULL);

	if (!fsm_addstate_bulk(fsm, 4)) { goto fail; }

	fsm_setstart(fsm, 0);
	if (!fsm_addedge_literal(fsm, 0, 1, 'a')) { goto fail; }

	if (!fsm_addedge_literal(fsm, 1, 2, 'b')) { goto fail; }
	if (!fsm_addedge_literal(fsm, 1, 3, 'c')) { goto fail; }

	if (!fsm_addedge_literal(fsm, 2, 2, 'b')) { goto fail; }
	if (!fsm_addedge_literal(fsm, 2, 3, 'c')) { goto fail; }

	fsm_setend(fsm, 3, 1);
	if (!fsm_setendid(fsm, 1)) {
		goto fail;
	}

	return fsm;

fail:
	exit(EXIT_FAILURE);
}

static void
check(const struct fsm *fsm, const char *string,
    unsigned expected_ends,
    unsigned cb_a, size_t pa_0, size_t pa_1,
    unsigned cb_b, size_t pb_0, size_t pb_1)
{
	int exec_res;
	size_t i;
	struct captest_input input;
	fsm_state_t end;
	struct fsm_capture captures[MAX_TEST_CAPTURES];

	fprintf(stderr, "#### check '%s', exp: ends 0x%u, c%u: (%ld, %ld), c%u: %ld, %ld)\n",
	    string, expected_ends,
	    cb_a, pa_0, pa_1,
	    cb_b, pb_0, pb_1);

	input.string = string;
	input.pos = 0;

	for (i = 0; i < MAX_TEST_CAPTURES; i++) {
		captures[i].pos[0] = FSM_CAPTURE_NO_POS;
		captures[i].pos[1] = FSM_CAPTURE_NO_POS;
	}

	exec_res = fsm_exec(fsm, captest_getc, &input, &end, captures);
	if (exec_res != 1) {
		fprintf(stderr, "fsm_exec: %d\n", exec_res);
		exit(EXIT_FAILURE);
	}

	/* check captures */
	fprintf(stderr, "captures for '%s': [%ld, %ld], [%ld, %ld]\n",
	    string,
	    captures[0].pos[0], captures[0].pos[1],
	    captures[1].pos[0], captures[1].pos[1]);
	assert(captures[cb_a].pos[0] == pa_0);
	assert(captures[cb_a].pos[1] == pa_1);
	assert(captures[cb_b].pos[0] == pb_0);
	assert(captures[cb_b].pos[1] == pb_1);

	{
		int gres;
		fsm_end_id_t ids[2];

		gres = fsm_endid_get(fsm, end, fsm_endid_count(fsm, end), ids);
		if (gres != 1) {
			assert(!"fsm_getendids failed");
		}

		if (expected_ends == 0x2) {
			assert(fsm_endid_count(fsm, end) == 1);
			assert(ids[0] == 1);
		} else if (expected_ends == 0x3) {
			assert(fsm_endid_count(fsm, end) == 2);
			assert(ids[0] == 0);
			assert(ids[1] == 1);
		} else {
			assert(!"test not handled");
		}
	}
}
