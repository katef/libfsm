/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */
#ifndef CAPTEST_H
#define CAPTEST_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/capture.h>
#include <fsm/options.h>

#define MAX_SINGLE_FSM_TEST_PATHS 8
#define MAX_TEST_CAPTURES 8

#define CAPTEST_RUN_SINGLE_LOG 0
#define LOG_INTERMEDIATE_FSMS 0

struct captest_single_fsm_test_info {
	const char *string;
	struct captest_single_fsm_test_path {
		fsm_state_t start;
		fsm_state_t end;
	} paths[MAX_SINGLE_FSM_TEST_PATHS];
};

struct captest_end_opaque {
#define CAPTEST_END_OPAQUE_TAG 'E'
	unsigned char tag;
	unsigned ends;		/* bit set */
};

struct captest_input {
	const char *string;
	size_t pos;
};

int
captest_run_single(const struct captest_single_fsm_test_info *info);

int
captest_getc(void *opaque);

struct fsm *
captest_fsm_with_options(void);

struct fsm *
captest_fsm_of_string(const char *string, unsigned end_id);

#endif
