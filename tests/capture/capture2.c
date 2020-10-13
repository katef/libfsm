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

#include "captest.h"

/* (a(b((c))(d))) */

int main(void) {
	struct captest_single_fsm_test_info test_info = {
		"abcd",
		{
			{ 0, 4 },
			{ 1, 4 },
			{ 2, 3 },
			{ 2, 3 },
			{ 3, 4 },
		}
	};
	return captest_run_single(&test_info);
}
