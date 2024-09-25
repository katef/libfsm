#ifndef TESTUTIL_H
#define TESTUTIL_H

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

#define DEF_STEP_LIMIT 100000

struct testcase {
	const char *regex;
	const char *required;
	size_t max_gen_buffer;	/* 0: default */
	size_t step_limit;
};

bool
run_test(const struct testcase *tc);

#endif
