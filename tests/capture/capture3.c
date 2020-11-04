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

#define LOG_INTERMEDIATE_FSMS 0

/* Combine 3 fully disjoint FSMs:
 *
 * - 0: "(ab)()"
 * - 1: "(cde)()"
 * - 2: "(fghi)()"
 *
 * Shift the captures for 1 and 2 forward and used/combine
 * opaques on them to track which one(s) matched. */

static void
check(const struct fsm *fsm, const char *string,
    unsigned end_id, unsigned capture_base);

static void
det_and_min(const char *tag, struct fsm *fsm);

int main(void) {
	struct fsm *f_ab = captest_fsm_of_string("ab", 0);
	struct fsm *f_cde = captest_fsm_of_string("cde", 1);
	struct fsm *f_fghi = captest_fsm_of_string("fghi", 2);
	struct fsm *f_all = NULL;
	unsigned captures;
	struct fsm_combine_info ci;
	unsigned cb_ab, cb_cde, cb_fghi; /* capture base */

	assert(f_ab);
	assert(f_cde);
	assert(f_fghi);

	/* set captures */
#define SET_CAPTURE(FSM, STATE, CAPTURE, TYPE)				\
	if (!fsm_set_capture_action(FSM, STATE, CAPTURE, TYPE)) {	\
		fprintf(stderr, "failed to set capture on line %d\n",	\
		    __LINE__);						\
		exit(EXIT_FAILURE);					\
	}

	/* (ab)() */
	if (!fsm_capture_set_path(f_ab, 0, 0, 2)) {
		exit(EXIT_FAILURE);
	}
	if (!fsm_capture_set_path(f_ab, 1, 2, 2)) {
		exit(EXIT_FAILURE);
	}

	/* (cde)() */
	if (!fsm_capture_set_path(f_cde, 0, 0, 3)) {
		exit(EXIT_FAILURE);
	}
	if (!fsm_capture_set_path(f_cde, 1, 3, 3)) {
		exit(EXIT_FAILURE);
	}

	/* (fghi)() */
	if (!fsm_capture_set_path(f_fghi, 0, 0, 4)) {
		exit(EXIT_FAILURE);
	}
	if (!fsm_capture_set_path(f_fghi, 1, 4, 4)) {
		exit(EXIT_FAILURE);
	}

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "\n=== f_ab...\n");
	fsm_print_fsm(stderr, f_ab);
	fsm_capture_dump(stderr, "#### f_ab", f_ab);

	fprintf(stderr, "\n=== f_cde...\n");
	fsm_print_fsm(stderr, f_cde);
	fsm_capture_dump(stderr, "#### f_cde", f_cde);

	fprintf(stderr, "\n=== f_fghi...\n");
	fsm_print_fsm(stderr, f_fghi);
	fsm_capture_dump(stderr, "#### f_fghi", f_fghi);
#endif

	/* determinise and minimise each before unioning */
	det_and_min("ab", f_ab);
	det_and_min("cde", f_cde);
	det_and_min("fghi", f_fghi);

	/* union them */
	f_all = fsm_union(f_ab, f_cde, &ci);
	assert(f_all != NULL);
	cb_ab = ci.capture_base_a;
	cb_cde = ci.capture_base_b;

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "=== unioned f_ab with f_cde... (CB ab: %u, cde: %u)\n",
	    cb_ab, cb_cde);
	fsm_print_fsm(stderr, f_all);
	fsm_capture_dump(stderr, "#### f_all", f_all);
#endif

	f_all = fsm_union(f_all, f_fghi, &ci);
	assert(f_all != NULL);
	cb_fghi = ci.capture_base_b;

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "=== unioned f_all with f_fghi... (CB all: %u, fghi: %u), %u captures\n",
	    ci.capture_base_a, cb_fghi, fsm_countcaptures(f_all));
	fsm_print_fsm(stderr, f_all);
	fsm_capture_dump(stderr, "#### f_all #2", f_all);
#endif

	if (!fsm_determinise(f_all)) {
		fprintf(stderr, "NOPE %d\n", __LINE__);
		exit(EXIT_FAILURE);
	}

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "==== after determinise\n");
	fsm_print_fsm(stderr, f_all);
	fsm_capture_dump(stderr, "#### f_all", f_all);
#endif

	if (!fsm_minimise(f_all)) {
		fprintf(stderr, "NOPE %d\n", __LINE__);
		exit(EXIT_FAILURE);
	}

#if LOG_INTERMEDIATE_FSMS
	fprintf(stderr, "==== after minimise\n");
	fsm_print_fsm(stderr, f_all);
	fsm_capture_dump(stderr, "#### f_all", f_all);
#endif

	captures = fsm_countcaptures(f_all);
	if (captures != 6) {
		fprintf(stderr, "expected 6 captures, got %u\n", captures);
		exit(EXIT_FAILURE);
	}

	check(f_all, "ab", 0, cb_ab);
	check(f_all, "cde", 1, cb_cde);
	check(f_all, "fghi", 2, cb_fghi);

	fsm_free(f_all);

	return 0;
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
	fsm_print_fsm(stderr, fsm);
	fsm_capture_dump(stderr, tag, fsm);
#endif

}

static void
check(const struct fsm *fsm, const char *string,
    unsigned end_id, unsigned capture_base)
{
	int exec_res;
	size_t i;
	struct captest_input input;
	fsm_state_t end;
	struct fsm_capture captures[MAX_TEST_CAPTURES];
	struct captest_end_opaque *eo = NULL;
	const size_t length = strlen(string);
	const unsigned cb = capture_base; /* alias */

	input.string = string;
	input.pos = 0;

	for (i = 0; i < MAX_TEST_CAPTURES; i++) {
		captures[i].pos[0] = FSM_CAPTURE_NO_POS;
		captures[i].pos[1] = FSM_CAPTURE_NO_POS;
	}

	exec_res = fsm_exec(fsm, captest_getc, &input, &end, captures);
	if (exec_res != 1) {
		fprintf(stderr, "fsm_exec: %d for '%s', expected 1\n",
		    exec_res, string);
		exit(EXIT_FAILURE);
	}

	/* check opaque end ID */
	eo = fsm_getopaque(fsm, end);
	fprintf(stderr, "exec: end %d -> %p\n", end, (void *)eo);

	assert(eo != NULL);
	assert(eo->tag == CAPTEST_END_OPAQUE_TAG);
	assert(eo->ends & (1U << end_id));

	/* check captures */
	fprintf(stderr, "captures for '%s' (cb %u): [%ld, %ld], [%ld, %ld]\n",
	    string, capture_base,
	    captures[0 + cb].pos[0], captures[0 + cb].pos[1],
	    captures[1 + cb].pos[0], captures[1 + cb].pos[1]);
	assert(captures[0 + cb].pos[0] == 0);
	assert(captures[0 + cb].pos[1] == length);
	assert(captures[1 + cb].pos[0] == length);
	assert(captures[1 + cb].pos[1] == length);
}
