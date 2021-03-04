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

#define LOG_INTERMEDIATE_FSMS 0
#include "captest.h"

/* Check that self edges are handled properly in the
 * capture action analysis.
 *
 * The DFA corresponds to /a(b*)(c)/. */

static struct fsm *
build(void);

static void
check(struct fsm *f, const char *input,
    unsigned pa_0, unsigned pa_1,
    unsigned pb_0, unsigned pb_1);

int main(void) {
	struct fsm *f = build();
	unsigned captures;
	assert(f != NULL);

	captures = fsm_countcaptures(f);
	if (captures != 2) {
		fprintf(stderr, "expected 2 captures, got %u\n", captures);
		exit(EXIT_FAILURE);
	}

	check(f, "ac",
	    1, 1,
	    1, 2);
	check(f, "abc",
	    1, 2,
	    2, 3);
	check(f, "abbc",
	    1, 3,
	    3, 4);

	fsm_free(f);

	return 0;
}

static struct fsm *
build(void)
{
	struct fsm *fsm = captest_fsm_with_options();

	if (!fsm_addstate_bulk(fsm, 4)) { goto fail; }

	fsm_setstart(fsm, 0);
	if (!fsm_addedge_literal(fsm, 0, 1, 'a')) { goto fail; }

	if (!fsm_addedge_literal(fsm, 1, 1, 'b')) { goto fail; }
	if (!fsm_addedge_literal(fsm, 1, 2, 'c')) { goto fail; }

	fsm_setend(fsm, 2, 1);

	if (!fsm_capture_set_path(fsm, 0, 1, 1)) { goto fail; }
	if (!fsm_capture_set_path(fsm, 1, 1, 2)) { goto fail; }

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "==== built\n");
	fsm_print_fsm(stderr, fsm);
	fsm_capture_dump(stderr, "built", fsm);
#endif

	if (!fsm_determinise(fsm)) {
		fprintf(stderr, "Failed to determise\n");
		exit(EXIT_FAILURE);
	}

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "==== after det\n");
	fsm_print_fsm(stderr, fsm);
	fsm_capture_dump(stderr, "after det", fsm);
#endif

	if (!fsm_minimise(fsm)) {
		fprintf(stderr, "Failed to minimise\n");
		exit(EXIT_FAILURE);
	}

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "==== after min\n");
	fsm_print_fsm(stderr, fsm);
	fsm_capture_dump(stderr, "after min", fsm);
#endif
	return fsm;

fail:
	exit(EXIT_FAILURE);
}

static void
check(struct fsm *fsm, const char *string,
    unsigned pa_0, unsigned pa_1,
    unsigned pb_0, unsigned pb_1)
{
	int exec_res;
	size_t i;
	struct captest_input input;
	fsm_state_t end;
	struct fsm_capture captures[MAX_TEST_CAPTURES];

	fprintf(stderr, "#### check '%s', exp: c%u: (%u, %u), c%u: %u, %u)\n",
	    string,
	    0, pa_0, pa_1,
	    1, pb_0, pb_1);

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
	assert(captures[0].pos[0] == pa_0);
	assert(captures[0].pos[1] == pa_1);
	assert(captures[1].pos[0] == pb_0);
	assert(captures[1].pos[1] == pb_1);
}
