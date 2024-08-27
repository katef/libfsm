#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <errno.h>

#include <assert.h>

#include <re/re.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/options.h>
#include <fsm/print.h>
#include <fsm/walk.h>

#define MAX_IDS 32

#include <assert.h>

#include <fsm/fsm.h>

#define MAX_PATTERNS 150
#define MAX_INPUTS 64
#define MAX_ENDIDS 32

struct eager_output_test {
	const char *patterns[MAX_PATTERNS];

	struct {
		const char *input;
		bool expect_fail;
		/* Terminated by 0. pattern[i] => id of i+1. Must be sorted. */
		fsm_output_id_t expected_ids[MAX_ENDIDS];
	} inputs[MAX_INPUTS];
};

void
append_eager_output_cb(fsm_output_id_t id, void *opaque);

int
cmp_output(const void *pa, const void *pb);

int
run_test(const struct eager_output_test *test, bool allow_extra_outputs, bool force_endids);

struct cb_info {
	size_t used;
	fsm_end_id_t ids[MAX_IDS];
};

void
dump(const struct fsm *fsm);

void
append_eager_output_cb(fsm_end_id_t id, void *opaque);

#endif
