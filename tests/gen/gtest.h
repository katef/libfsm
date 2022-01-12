#ifndef GTEST_H
#define GTEST_H

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/walk.h>

struct exp_match {
	const char *s;
	size_t length;
	bool found;
	bool prune;
};

struct exp_matches {
	unsigned magic;
	size_t count;
	struct exp_match *matches;
};

/* random magic number */
#define EXP_MATCHES_MAGIC 0xc01f8914UL

#define BOX_MATCHES(VAR, MATCHES)				\
	struct exp_matches VAR = {				\
		.magic = EXP_MATCHES_MAGIC,			\
		.count = sizeof(MATCHES)/sizeof(MATCHES)[0],	\
		.matches = MATCHES,				\
	};							\
	gtest_init_matches(&VAR)

void
gtest_init_matches(struct exp_matches *ms);

enum fsm_generate_matches_cb_res
gtest_matches_cb(const struct fsm *fsm,
    size_t depth, size_t match_count, size_t steps,
    const char *input, size_t input_length,
    fsm_state_t end_state, void *opaque);

/* Bulid an FSM by combining DFAs for a set of string literals. */
struct fsm *
gtest_fsm_of_matches(const struct exp_matches *ms);

bool
gtest_all_found(const struct exp_matches *ms, FILE *f);

#endif
