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
#include <fsm/walk.h>

#include "gtest.h"

int main(void) {
#define MAX_EXP_MATCH 4
	struct exp_match ends[] = {
		{ .s = "abc", },
		{ .s = "aba", },
		{ .s = "abac", },
		{ .s = "aca", },
		{ .s = "acab", },
		{ .s = "bb", },
		{ .s = "bba", },
		{ .s = "babc", },
		{ .s = "z", },
		{ .s = "", },
	};
	BOX_MATCHES(matches, ends);

	struct fsm *fsm = gtest_fsm_of_matches(&matches);
	assert(fsm != NULL);

	if (!fsm_generate_matches(fsm, MAX_EXP_MATCH + 1 /* for \0 */,
		gtest_matches_cb, &matches)) {
		fprintf(stderr, "fsm_generate_matches: error\n");
		exit(EXIT_FAILURE);
	}

	fsm_free(fsm);

	if (!gtest_all_found(&matches, stderr)) {
		exit(EXIT_FAILURE);
	}

	return 0;
}
