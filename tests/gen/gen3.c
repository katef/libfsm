/*
 * Copyright 2021 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/print.h>
#include <fsm/walk.h>

#include "gtest.h"

#ifndef LOG_MATCHES
#define LOG_MATCHES 0
#endif

static struct fsm *
build(void)
{
	struct fsm *fsm = fsm_new(NULL);
	if (!fsm_addstate_bulk(fsm, 4)) {
		return NULL;
	}

	fsm_setstart(fsm, 0);

#define LITERAL(FROM, TO, LABEL)				\
	if (!fsm_addedge_literal(fsm, FROM, TO, LABEL)) {	\
	    exit(EXIT_FAILURE);					\
	}

#define EPSILON(FROM, TO)					\
	if (!fsm_addedge_epsilon(fsm, FROM, TO)) {		\
	    exit(EXIT_FAILURE);					\
	}

	/* a*bc*de(fde)* */
	LITERAL(0, 0, 'a');
	LITERAL(0, 1, 'b');
	LITERAL(1, 1, 'c');
	LITERAL(1, 2, 'd');
	EPSILON(2, 3);
	EPSILON(3, 2);
	LITERAL(2, 3, 'e');
	LITERAL(3, 1, 'f');

	fsm_setend(fsm, 3, 1);
	fsm_setendid(fsm, 1);

	/* fsm_generate_matches requires a DFA. */
	if (!fsm_determinise(fsm)) {
		exit(EXIT_FAILURE);
	}
	if (!fsm_minimise(fsm)) {
		exit(EXIT_FAILURE);
	}

	if (getenv("PRINT")) {
		fsm_print_fsm(stderr, fsm);
	}

	return fsm;
}

static struct exp_match exp_matches[] = {
	/* this is just a subset */
	{ .s = "bde" },
	{ .s = "bdefde" },
	{ .s = "bdefdefcde" },
	{ .s = "bdefcccde" },

	/* prune after this one, so "abcdefde" does NOT appear  */
	{ .s = "abcde", .prune = true },

	{ .s = "bcde" },
	{ .s = "bccde" },
	{ .s = "abdefdefde" },
	{ .s = "aabccdefde" },
	{ .s = "aabcccccde" },
	{ .s = "aaaaaabcde" },
};
static size_t exp_matches_count = sizeof(exp_matches)/sizeof(exp_matches[0]);

static const char *exp_not_present = "abcdefde";

static bool
all_found(void)
{
	for (size_t i = 0; i < exp_matches_count; i++) {
		if (!exp_matches[i].found) {
			fprintf(stderr, "NOT FOUND: \"%s\"\n",
			    exp_matches[i].s);
			return false;
		}
	}

	return true;
}

enum fsm_generate_matches_cb_res
matches_cb(const struct fsm *fsm,
    size_t depth, size_t match_count, size_t steps,
    const char *input, size_t input_length,
    fsm_state_t end_state, void *opaque)
{
	(void)fsm;
	(void)opaque;

	if (LOG_MATCHES) {
		fprintf(stderr,
		    "matches_cb: depth %zu, match_count %zu, steps %zu, input \"%s\"(%zu), end_state %d\n",
		    depth, match_count, steps, input, input_length, end_state);
	}

	if (0 == strcmp(exp_not_present, input)) {
		fprintf(stderr,
		    "ERROR: Should have been pruned: \"%s\"(%zu)\n",
		    exp_not_present, input_length);
		exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < exp_matches_count; i++) {
		struct exp_match *m = &exp_matches[i];
		if (0 == strcmp(input, m->s)) {
			/* assert(!m->found); */
			m->found = true;

			if (m->prune) {
				return FSM_GENERATE_MATCHES_CB_RES_PRUNE;
			}

			break;
		}
	}

	return FSM_GENERATE_MATCHES_CB_RES_CONTINUE;
}

int main(void) {
	struct fsm *fsm = build();
	assert(fsm != NULL);

	if (!fsm_generate_matches(fsm, 11, matches_cb, NULL)) {
		fprintf(stderr, "fsm_generate_matches: error\n");
		exit(EXIT_FAILURE);
	}

	fsm_free(fsm);

	if (!all_found()) {
		exit(EXIT_FAILURE);
	}

	return 0;
}
